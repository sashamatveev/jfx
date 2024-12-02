/*
 * Copyright (c) 2024, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <new>
#include <string.h>
#include <stdio.h>

#include <Mferror.h>

#include "mfdemux.h"

//#include <mfidl.h>
//#include <Wmcodecdsp.h>


using namespace std;

#define MAX_CODEC_DATA_SIZE 256

GST_DEBUG_CATEGORY_STATIC(gst_mfdemux_debug);
#define GST_CAT_DEFAULT gst_mfdemux_debug

// The input capabilities
static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        "video/mp4;"
        "video/quicktime;"
        "audio/x-m4a;"
        "video/x-m4v"
    ));

// The output capabilities
static GstStaticPadTemplate src_video_factory =
GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_audio_factory =
GST_STATIC_PAD_TEMPLATE ("audio",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

// Forward declarations
static void gst_mfdemux_dispose(GObject *object);

static void gst_mfdemux_set_property(GObject *object, guint property_id,
                                     const GValue *value, GParamSpec *pspec);
static void gst_mfdemux_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec);

static GstFlowReturn mfdemux_chain(GstPad *pad, GstObject *parent, GstBuffer *buf);
static void mfdemux_loop(GstPad *pad);

static gboolean mfdemux_sink_event(GstPad *pad, GstObject *parent, GstEvent *event);
static gboolean mfdemux_sink_set_caps(GstPad *pad, GstObject *parent, GstCaps *caps);

static gboolean mfdemux_src_query (GstPad *pad, GstObject *parent, GstQuery *query);
static gboolean mfdemux_src_event (GstPad *pad, GstObject *parent, GstEvent *event);

static gboolean mfdemux_activate(GstPad *pad, GstObject *parent);
static gboolean mfdemux_activate_mode(GstPad *pad, GstObject *parent,
                                      GstPadMode mode, gboolean active);

//static HRESULT mfdemux_load_demux(GstMFDemux *demux, GstCaps *caps);

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

/***********************************************************************************
* Substitution for
* G_DEFINE_TYPE (GstMFDemux, gst_mfdemux, GstElement, GST_TYPE_ELEMENT);
***********************************************************************************/
#define gst_mfdemux_parent_class parent_class
static void gst_mfdemux_init(GstMFDemux *self);
static void gst_mfdemux_class_init(GstMFDemuxClass *klass);
static gpointer gst_mfdemux_parent_class = NULL;
static void gst_mfdemux_class_intern_init(gpointer klass)
{
    gst_mfdemux_parent_class = g_type_class_peek_parent(klass);
    gst_mfdemux_class_init((GstMFDemuxClass*)klass);
}

GType gst_mfdemux_get_type(void)
{
    static volatile gsize gonce_data = 0;
    // INLINE - g_once_init_enter()
    if (g_once_init_enter(&gonce_data))
    {
        GType _type;
        _type = g_type_register_static_simple(GST_TYPE_ELEMENT,
            g_intern_static_string("GstMFDemux"),
            sizeof(GstMFDemuxClass),
            (GClassInitFunc)gst_mfdemux_class_intern_init,
            sizeof(GstMFDemux),
            (GInstanceInitFunc)gst_mfdemux_init,
            (GTypeFlags)0);
        g_once_init_leave(&gonce_data, (gsize)_type);
    }
    return (GType)gonce_data;
}

// Initialize mfdemux's class.
static void gst_mfdemux_class_init(GstMFDemuxClass *klass)
{
    GstElementClass *element_class = (GstElementClass*)klass;
    GObjectClass *gobject_class = (GObjectClass*)klass;

    gst_element_class_set_metadata(element_class,
        "MFDemux",
        "Codec/Decoder/Audio/Video",
        "Media Foundation Demux",
        "Oracle Corporation");

    gst_element_class_add_pad_template(element_class,
        gst_static_pad_template_get(&src_video_factory));
    gst_element_class_add_pad_template(element_class,
        gst_static_pad_template_get(&src_audio_factory));
    gst_element_class_add_pad_template(element_class,
        gst_static_pad_template_get(&sink_factory));

    gobject_class->dispose = gst_mfdemux_dispose;
    gobject_class->set_property = gst_mfdemux_set_property;
    gobject_class->get_property = gst_mfdemux_get_property;

    // g_object_class_install_property(gobject_class, PROP_CODEC_ID,
    //     g_param_spec_int("codec-id", "Codec ID", "Codec ID", -1, G_MAXINT, 0,
    //     (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS)));

    // g_object_class_install_property(gobject_class, PROP_IS_SUPPORTED,
    //     g_param_spec_boolean("is-supported", "Is supported", "Is codec ID supported", FALSE,
    //     (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS)));
}

// Initialize the new element
// Instantiate pads and add them to element
// Set pad calback functions
// Initialize instance structure
static void gst_mfdemux_init(GstMFDemux *demux)
{
    g_mutex_init(&demux->lock);

    demux->src_result = GST_FLOW_OK;

    // Input
    demux->sink_pad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_chain_function(demux->sink_pad, mfdemux_chain);
    gst_pad_set_event_function(demux->sink_pad, mfdemux_sink_event);
    gst_pad_set_activate_function(demux->sink_pad, mfdemux_activate);
    gst_pad_set_activatemode_function(demux->sink_pad, mfdemux_activate_mode);
    gst_element_add_pad(GST_ELEMENT(demux), demux->sink_pad);

    // // Output
    // demux->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    // gst_element_add_pad(GST_ELEMENT(demux), demux->srcpad);

    demux->is_flushing = FALSE;
    demux->is_eos_received = FALSE;
    demux->is_eos = FALSE;
    demux->is_demux_initialized = FALSE;
    demux->force_discontinuity = FALSE;
    demux->send_new_segment = FALSE;
    // demux->force_output_discontinuity = FALSE;

    demux->rate = 1.0;
    demux->seek_position = 0;

    demux->pGSTMFByteStream = NULL;
    demux->pIMFByteStream = NULL;
    demux->pSourceReader = NULL;

    demux->llDuration = -1;

    // Init audio format with some defaults
    demux->audioFormat.codecID = JFX_CODEC_ID_UNKNOWN;
    demux->audioFormat.uiChannels = 2;
    demux->audioFormat.uiRate = 48000;
    demux->audioFormat.codec_data = NULL;

    // Init video format with some defaults
    demux->videoFormat.codecID = JFX_CODEC_ID_UNKNOWN;
    demux->videoFormat.uiWidth = 1920;
    demux->videoFormat.uiHeight = 1080;
    demux->videoFormat.codec_data = NULL;

    demux->audio_src_pad = NULL;
    demux->video_src_pad = NULL;

    demux->audio_stream_index = -1;
    demux->video_stream_index = -1;

    demux->cached_segment_event = NULL;

    // Initialize Media Foundation
    bool bCallCoUninitialize = true;

    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        bCallCoUninitialize = false;

    demux->hr_mfstartup = MFStartup(MF_VERSION, MFSTARTUP_LITE);

    if (bCallCoUninitialize)
        CoUninitialize();

    // demux->pDecoder = NULL;
    // demux->pDecoderOutput = NULL;

    // demux->pColorConvert = NULL;
    // demux->pColorConvertOutput = NULL;

    // demux->header = NULL;
    // demux->header_size = 0;

    // demux->width = 1920;
    // demux->height = 1080;
    // demux->framerate_num = 2997;
    // demux->framerate_den = 100;
}

static void gst_mfdemux_dispose(GObject* object)
{
    GstMFDemux *demux = GST_MFDEMUX(object);

    g_mutex_clear(&demux->lock);

    SafeRelease(&demux->pSourceReader);
    SafeRelease(&demux->pIMFByteStream);

    // SafeRelease(&demux->pDecoderOutput);
    // SafeRelease(&demux->pDecoder);

    // SafeRelease(&demux->pColorConvertOutput);
    // SafeRelease(&demux->pColorConvert);

    if (demux->audioFormat.codec_data != NULL)
    {
        // INLINE - gst_buffer_unref()
        gst_buffer_unref(demux->audioFormat.codec_data);
        demux->audioFormat.codec_data = NULL;
    }

    if (demux->videoFormat.codec_data != NULL)
    {
        // INLINE - gst_buffer_unref()
        gst_buffer_unref(demux->videoFormat.codec_data);
        demux->videoFormat.codec_data = NULL;
    }

    if (demux->cached_segment_event != NULL)
    {
        // INLINE - gst_event_unref()
        gst_event_unref(demux->cached_segment_event);
        demux->cached_segment_event = NULL;
    }

    if (demux->hr_mfstartup == S_OK)
        MFShutdown();

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_mfdemux_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    g_print("AMDEBUG gst_mfdemux_set_property()\n");
    // GstMFDemux *demux = GST_MFDEMUX(object);
    // switch (property_id)
    // {
    // case PROP_CODEC_ID:
    //     demux->codec_id = g_value_get_int(value);
    //     break;
    // default:
    //     break;
    // }
}

static void gst_mfdemux_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    g_print("AMDEBUG gst_mfdemux_get_property()\n");
    //g_value_set_boolean(value, FALSE);
    // GstMFDemux *demux = GST_MFDEMUX(object);
    // gboolean is_supported = FALSE;
    // switch (property_id)
    // {
    // case PROP_IS_SUPPORTED:
    //     is_supported = mfdemux_is_demux_by_codec_id_supported(demux, demux->codec_id);
    //     g_value_set_boolean(value, is_supported);
    //     break;
    // default:
    //     break;
    // }
}

// Processes input buffers
static GstFlowReturn mfdemux_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    // INLINE - gst_buffer_unref()
    gst_buffer_unref(buf);
    return GST_FLOW_NOT_SUPPORTED;
}

static gboolean mfdemux_push_sink_event(GstMFDemux *demux, GstEvent *event)
{
    gboolean ret = TRUE;

    if (demux->audio_src_pad != NULL && gst_pad_is_linked(demux->audio_src_pad))
        ret = gst_pad_push_event(demux->audio_src_pad, gst_event_ref(event));  // INLINE - gst_event_ref()

    if (demux->video_src_pad != NULL && gst_pad_is_linked(demux->video_src_pad))
        ret = gst_pad_push_event(demux->video_src_pad, gst_event_ref(event));  // INLINE - gst_event_ref()

    // INLINE - gst_event_unref()
    gst_event_unref(event);

    return ret;
}

static gboolean mfdemux_sink_event(GstPad* pad, GstObject *parent, GstEvent *event)
{
    gboolean ret = FALSE;
    GstMFDemux *demux = GST_MFDEMUX(parent);
    HRESULT hr = S_OK;

    switch (GST_EVENT_TYPE(event))
    {
    case GST_EVENT_SEGMENT:
    {
        demux->force_discontinuity = TRUE;
        demux->is_eos_received = FALSE;
        demux->is_eos = FALSE;

        if (demux->pGSTMFByteStream)
            demux->pGSTMFByteStream->ClearEOS();

        // Cache segment event if we not ready yet
        if ((demux->audio_src_pad != NULL && gst_pad_is_linked(demux->audio_src_pad)) ||
            (demux->video_src_pad != NULL && gst_pad_is_linked(demux->video_src_pad)))
        {
            ret = mfdemux_push_sink_event(demux, event);
        }
        else
        {
            demux->cached_segment_event = event;
            ret = TRUE;
        }
    }
    break;
    case GST_EVENT_FLUSH_START:
    {
        // INLINE - gst_event_unref()
        gst_event_unref(event);
        ret = TRUE;
    }
    break;
    case GST_EVENT_FLUSH_STOP:
    {
        // INLINE - gst_event_unref()
        gst_event_unref(event);
        ret = TRUE;
    }
    break;
    case GST_EVENT_EOS:
    {
        demux->is_eos_received = TRUE;
        demux->is_eos = TRUE;

        if (demux->pGSTMFByteStream)
            demux->pGSTMFByteStream->SignalEOS();

        // INLINE - gst_event_unref()
        gst_event_unref(event);
        ret = TRUE;
    }
    break;
    case GST_EVENT_CAPS:
    {
        GstCaps *caps;

        gst_event_parse_caps(event, &caps);
        if (!mfdemux_sink_set_caps(pad, parent, caps))
        {
            gst_element_message_full(GST_ELEMENT(demux), GST_MESSAGE_ERROR, GST_STREAM_ERROR, GST_STREAM_ERROR_DECODE, g_strdup("mfdemux_sink_set_caps() failed"), NULL, ("mfdemux.c"), ("mfdemux_sink_event"), 0);
        }

        // INLINE - gst_event_unref()
        gst_event_unref(event);
        ret = TRUE;
    }
    break;
    // This event appears only in pull mode during outrange reading or seeking.
    case FX_EVENT_RANGE_READY:
    {
        if (demux->pGSTMFByteStream)
            demux->pGSTMFByteStream->ReadRangeAvailable();

        // INLINE - gst_event_unref()
        gst_event_unref(event);
        ret = TRUE;
    }
    case FX_EVENT_SEGMENT_READY:
    {
        gint64 size = -1;
        const GstStructure *s = gst_event_get_structure(event);
        if (s != NULL)
        {
            if (!gst_structure_get_int64(s, "size", &size))
                size = -1;
        }

        if (demux->pGSTMFByteStream)
        {
            demux->pGSTMFByteStream->SetSegmentLength((QWORD)size, false);
            demux->pGSTMFByteStream->ReadRangeAvailable();
        }

        // INLINE - gst_event_unref()
        gst_event_unref(event);
        ret = TRUE;
    }
    break;
    default:
        ret = mfdemux_push_sink_event(demux, event);
        break;
    }

    return ret;
}

static gboolean mfdemux_sink_set_caps(GstPad * pad, GstObject *parent, GstCaps * caps)
{
    gboolean ret = TRUE;
    GstMFDemux *demux = GST_MFDEMUX(parent);

    if (pad == demux->sink_pad)
    {
        // TODO Check caps
        //ret = mfdemux_init_demux(demux, caps);
    }

    return ret;
}

static gboolean mfdemux_src_query(GstPad *pad, GstObject *parent, GstQuery *query)
{
    gboolean ret = TRUE;
    GstMFDemux *demux = GST_MFDEMUX(parent);

    switch (GST_QUERY_TYPE(query))
    {
        case GST_QUERY_DURATION:
        {
            GstFormat format;

            gst_query_parse_duration(query, &format, NULL);
            if (format != GST_FORMAT_TIME || demux->llDuration == -1)
                ret = gst_pad_query_default(pad, parent, query);
            else
                gst_query_set_duration(query, GST_FORMAT_TIME, demux->llDuration * 100);
        }
        break;
        default:
            ret = gst_pad_query_default(pad, parent, query);
    }

    return ret;
}

static gboolean mfdemux_src_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    gboolean ret = FALSE;
    GstMFDemux *demux = GST_MFDEMUX(parent);

    switch (GST_EVENT_TYPE (event))
    {
        case GST_EVENT_SEEK:
        {
            HRESULT hr = S_OK;
            gdouble rate;           // segment rate
            GstFormat format;       // format of the seek values
            GstSeekFlags flags;     // the seek flags
            GstSeekType start_type; // the seek type of the start position
            GstSeekType stop_type;  // the seek type of the stop position
            gint64 start;           // the seek start position in the given format
            gint64 stop;            // the seek stop position in the given format
            guint32 seqnum;

            // Do not init seek if we in error state. It can happen if
            // critical error occured and we disposing pipeline.
            g_mutex_lock(&demux->lock);
            if (demux->src_result == GST_FLOW_ERROR ||
                demux->pGSTMFByteStream == NULL ||
                demux->pSourceReader == NULL)
            {
                g_mutex_unlock(&demux->lock);
                // INLINE - gst_event_unref()
                gst_event_unref (event);
                return TRUE;
            }
            g_mutex_unlock(&demux->lock);

            // Clear EOS on byte stream, since SourceReader will start
            // reading it during seek.
            if (demux->pGSTMFByteStream)
                demux->pGSTMFByteStream->ClearEOS();

            // Get seek description from the event.
            gst_event_parse_seek (event, &rate, &format, &flags,
                    &start_type, &start, &stop_type, &stop);
            seqnum = gst_event_get_seqnum(event);
            if (format == GST_FORMAT_TIME)
            {
                if (flags & GST_SEEK_FLAG_FLUSH)
                {
                    GstEvent *e = gst_event_new_flush_start();
                    gst_event_set_seqnum(e, seqnum);
                    // Push event dowstream. We do not flush upstream, since
                    // we working in pull mode.
                    mfdemux_push_sink_event(demux, e);
                }

                // Stop streaming thread
                g_mutex_lock(&demux->lock);
                demux->src_result = GST_FLOW_FLUSHING;
                g_mutex_unlock(&demux->lock);

                // Lock pad. Streaming thread might be waiting for data, but
                // it should release stream lock when doing it.
                GST_PAD_STREAM_LOCK(demux->sink_pad);
                // Unblock source reader if it was waiting for read.
                hr = demux->pSourceReader->Flush(MF_SOURCE_READER_ALL_STREAMS);
                // Unlock stream lock so streaming thread can continue.
                GST_PAD_STREAM_UNLOCK(demux->sink_pad);

                // Wait for streaming thread to exit
                gst_pad_pause_task(demux->sink_pad);

                if (demux->pGSTMFByteStream->IsSeekSupported())
                {
                    demux->rate = rate;
                    demux->seek_position = start;
                    demux->send_new_segment = TRUE;

                    PROPVARIANT pv = { 0 };
                    pv.vt = VT_I8;
                    pv.hVal.QuadPart = (LONGLONG)(start / 100);
                    hr = demux->pSourceReader->SetCurrentPosition(GUID_NULL, pv);
                    // TODO handle error
                    PropVariantClear(&pv);

                    // INLINE - gst_event_unref()
                    gst_event_unref (event);
                    ret = TRUE; // We handle event
                }
                else
                {
                    demux->pGSTMFByteStream->SetSegmentLength(-1, true);
                    // Upstream will handle and unref event
                    ret = gst_pad_push_event(demux->sink_pad, event);

                    PROPVARIANT pv = { 0 };
                    pv.vt = VT_I8;
                    //pv.hVal.QuadPart = (LONGLONG)(start / 100);
                    pv.hVal.QuadPart = (LONGLONG)(0);
                    hr = demux->pSourceReader->SetCurrentPosition(GUID_NULL, pv);
                    // TODO handle error
                    PropVariantClear(&pv);
                }

                if (flags & GST_SEEK_FLAG_FLUSH)
                {
                    GstEvent *e = gst_event_new_flush_stop(TRUE);
                    gst_event_set_seqnum(e, seqnum);
                    mfdemux_push_sink_event(demux, e);
                }

                // Start streaming thread
                g_mutex_lock(&demux->lock);
                demux->src_result = GST_FLOW_OK;
                g_mutex_unlock(&demux->lock);

                gst_pad_start_task(demux->sink_pad, (GstTaskFunction) mfdemux_loop,
                        demux->sink_pad, NULL);
            }
        }
        break;
        default:
            ret = gst_pad_push_event(demux->sink_pad, event);
            break;
    }

    return ret;
}

static gboolean mfdemux_init_demux(GstMFDemux *demux, GstCaps *caps)
{
    if (demux->is_demux_initialized)
        return TRUE;

    HRESULT hr = S_OK;

    gint64 data_length = 0;
    if (!gst_pad_peer_query_duration(demux->sink_pad, GST_FORMAT_BYTES, &data_length))
        data_length = -1; // -1 if unknown for MF (QWORD is ULONGLONG)
    else
        demux->send_new_segment = TRUE; // Lenght is know, which means it is
        // HTTP/FILE, so we need to provide segment. HLS will send it is own.

    demux->pGSTMFByteStream = new (nothrow) CGSTMFByteStream((QWORD)data_length, demux->sink_pad);
    if (demux->pGSTMFByteStream == NULL)
        return FALSE;

    hr = demux->pGSTMFByteStream->QueryInterface(IID_IMFByteStream, (void**)&demux->pIMFByteStream);
    if (FAILED(hr) || demux->pIMFByteStream == NULL)
        return FALSE;

    hr = MFCreateSourceReaderFromByteStream(demux->pIMFByteStream, NULL, &demux->pSourceReader);
    if (FAILED(hr) || demux->pSourceReader == NULL)
        return FALSE;

    // Get duration
    PROPVARIANT pv = {0};
    hr = demux->pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &pv);
    if (SUCCEEDED(hr))
    {
        demux->llDuration = (LONGLONG)pv.uhVal.QuadPart;
        PropVariantClear(&pv);
    }

    // Disable all streams. Disabled streams does not consume memory if not
    // read. MP4 might contain subtitles or additional audio stream and we do
    // not support it. We will enable needed streams when configuring demux.
    hr = demux->pSourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    if (FAILED(hr))
        return FALSE;

    demux->is_demux_initialized = TRUE;

    return TRUE;
}

// JFX_CODEC_ID_AAC (MFAudioFormat_AAC):
// From https://learn.microsoft.com/en-us/windows/win32/medfound/aac-media-types
// pBlobBytes contains the portion of the HEAACWAVEINFO structure that appears after
// the WAVEFORMATEX structure (that is, after the wfx member).
// This is followed by the AudioSpecificConfig() data, as defined by ISO/IEC 14496-3.
static gboolean mfdemux_extract_codec_data(GstMFDemux *demux, JFX_CODEC_ID codecID,
                                           UINT8 *pBlobBytes, UINT32 cbBlobSize,
                                           UINT8 *pCodecData, UINT32 *cbCodecDataSize)
{
    if (demux == NULL)
        return false;
    if (pBlobBytes == NULL || cbBlobSize <= 0)
        return false;
    if (pCodecData == NULL || cbCodecDataSize == NULL)
        return false;
    if ((*cbCodecDataSize) != MAX_CODEC_DATA_SIZE)
        return false;

    if (codecID == JFX_CODEC_ID_AAC &&
        cbBlobSize > (sizeof(HEAACWAVEINFO) - sizeof(WAVEFORMATEX)))
    {
        DWORD offset = sizeof(HEAACWAVEINFO) - sizeof(WAVEFORMATEX);

        if ((*cbCodecDataSize) >= (cbBlobSize - offset))
            (*cbCodecDataSize) = cbBlobSize - offset;
        else
            return false; // Not enough space in pCodecData buffer

        memcpy(pCodecData, pBlobBytes + offset, (*cbCodecDataSize));
    }
    else
    {
        return false;
    }

    return true;
}

// If codec_data not available or fail to read, then codec_data will be set to NULL.
// We will attempt to playback media stream without codec data anyway.
static void mfdemux_get_codec_data(GstMFDemux *demux, const GUID &guidKey,
                                   IMFMediaType *pMediaType, GstBuffer **codec_data, JFX_CODEC_ID codecID)
{
    UINT32 cbBlobSize = 0;
    UINT8 blobBytes[MAX_CODEC_DATA_SIZE] = {0};
    UINT32 cbCodecDataSize = MAX_CODEC_DATA_SIZE;
    UINT8 codecDataBytes[MAX_CODEC_DATA_SIZE] = {0};

    if (pMediaType == NULL)
        return;

    HRESULT hr = pMediaType->GetBlobSize(guidKey, &cbBlobSize);
    if (SUCCEEDED(hr) && cbBlobSize > 0 && cbBlobSize <= MAX_CODEC_DATA_SIZE)
    {
        hr = pMediaType->GetBlob(guidKey, &blobBytes[0], cbBlobSize, NULL);
        if (SUCCEEDED(hr))
        {
            if (!mfdemux_extract_codec_data(demux, codecID,
                    &blobBytes[0], cbBlobSize, &codecDataBytes[0], &cbCodecDataSize))
                return;
        }

        if (SUCCEEDED(hr))
            (*codec_data) = gst_buffer_new_allocate(NULL, (gsize)cbCodecDataSize, NULL);

        if ((*codec_data) != NULL)
        {
            GstMapInfo info;
            if (gst_buffer_map((*codec_data), &info, GST_MAP_READWRITE))
            {
                if (memcpy_s(info.data, info.maxsize, &codecDataBytes[0], cbCodecDataSize) != 0)
                    hr = E_FAIL;

                gst_buffer_unmap((*codec_data), &info);
                if (SUCCEEDED(hr))
                    return;
            }

            // INLINE - gst_buffer_unref()
            gst_buffer_unref((*codec_data));
            (*codec_data) = NULL;
        }
    }
}

static gboolean mfdemux_configure_audio_stream(GstMFDemux *demux, gboolean *hasAudio)
{
    HRESULT hr = S_OK;
    IMFMediaType *pMediaType = NULL;
    GUID subType;

    hr = demux->pSourceReader->
        SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    if (hr == MF_E_INVALIDSTREAMNUMBER)
    {
        (*hasAudio) = false;
        return TRUE;
    }

    (*hasAudio) = true;

    if (SUCCEEDED(hr))
    {
        hr = demux->pSourceReader->
            GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                               (DWORD)MF_SOURCE_READER_CURRENT_TYPE_INDEX,
                               &pMediaType);
    }

    if (SUCCEEDED(hr))
        hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &subType);

    if (IsEqualGUID(subType, MFAudioFormat_AAC)) {
        demux->audioFormat.codecID = JFX_CODEC_ID_AAC;
    } else {
        // Disable if format is not known
        hr = demux->pSourceReader->
            SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    }

    if (SUCCEEDED(hr) && demux->audioFormat.codecID == JFX_CODEC_ID_AAC)
    {
        pMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,
                &demux->audioFormat.uiChannels);
        pMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,
                &demux->audioFormat.uiRate);
        mfdemux_get_codec_data(demux, MF_MT_USER_DATA, pMediaType,
                &demux->audioFormat.codec_data,
                demux->audioFormat.codecID);
    }

    SafeRelease(&pMediaType);

    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static gboolean mfdemux_configure_audio_src_caps(GstMFDemux *demux)
{
    gboolean ret = FALSE;
    GstCaps *caps = NULL;
    GstEvent *caps_event = NULL;

    if (demux->audioFormat.codecID != JFX_CODEC_ID_AAC)
        return FALSE; // We should not be called with unsupported codec

    caps = gst_caps_new_simple ("audio/mpeg",
            "mpegversion", G_TYPE_INT, 4,
            "rate", G_TYPE_INT, (gint)demux->audioFormat.uiRate,
            "channels", G_TYPE_INT, (gint)demux->audioFormat.uiChannels,
            NULL);
    if (caps == NULL)
        return FALSE;

    if (demux->audioFormat.codec_data)
        gst_caps_set_simple(caps, "codec_data", GST_TYPE_BUFFER,
                            demux->audioFormat.codec_data, NULL);

    caps_event = gst_event_new_caps(caps);
    if (caps_event)
        ret = gst_pad_push_event(demux->audio_src_pad, caps_event);
    gst_caps_unref(caps);

    return ret;
}

static gboolean mfdemux_configure_audio_src_pad(GstMFDemux *demux)
{
    if (!demux)
        return FALSE;

    if (demux->audio_src_pad)
        return TRUE;

    if (demux->audioFormat.codecID != JFX_CODEC_ID_AAC)
        return TRUE; // Just ignore unknown audio stream

    demux->audio_src_pad =
            gst_pad_new_from_template(gst_element_class_get_pad_template
            (GST_ELEMENT_GET_CLASS(demux), "audio"), "audio");
    if (demux->audio_src_pad == NULL)
        return FALSE;

    gst_pad_set_query_function(demux->audio_src_pad, mfdemux_src_query);
    gst_pad_set_event_function(demux->audio_src_pad, mfdemux_src_event);

    if (!gst_pad_set_active(demux->audio_src_pad, TRUE) ||
        !mfdemux_configure_audio_src_caps(demux))
    {
        gst_object_unref(demux->audio_src_pad);
        demux->audio_src_pad = NULL;
        return FALSE;
    }

    gst_pad_use_fixed_caps(demux->audio_src_pad);

    if (!gst_element_add_pad(GST_ELEMENT(demux), demux->audio_src_pad)) {
        // Pad will be unref even if gst_element_add_pad() fails
        demux->audio_src_pad = NULL;
        return FALSE;
    }

    return TRUE;
}

static gboolean mfdemux_configure_video_stream(GstMFDemux *demux, gboolean *hasVideo)
{
    HRESULT hr = S_OK;
    IMFMediaType *pMediaType = NULL;
    GUID subType;

    hr = demux->pSourceReader->
        SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    if (hr == MF_E_INVALIDSTREAMNUMBER)
    {
        (*hasVideo) = false;
        return TRUE;
    }

    (*hasVideo) = true;

    if (SUCCEEDED(hr))
    {
        hr = demux->pSourceReader->
            GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                               (DWORD)MF_SOURCE_READER_CURRENT_TYPE_INDEX,
                               &pMediaType);
    }

    if (SUCCEEDED(hr))
        hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &subType);

    if (IsEqualGUID(subType, MFVideoFormat_H264))
    {
        demux->videoFormat.codecID = JFX_CODEC_ID_H264;
    }
    else if (IsEqualGUID(subType, MFVideoFormat_HEVC))
    {
        demux->videoFormat.codecID = JFX_CODEC_ID_HEVC;
    }
    else
    {
        // Disable if format is not known
        hr = demux->pSourceReader->
            SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    }

    if (SUCCEEDED(hr) && (demux->videoFormat.codecID == JFX_CODEC_ID_H264 ||
                        demux->videoFormat.codecID == JFX_CODEC_ID_HEVC))
    {
        MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE,
                           &demux->videoFormat.uiWidth,
                           &demux->videoFormat.uiHeight);
        mfdemux_get_codec_data(demux, MF_MT_MPEG_SEQUENCE_HEADER, pMediaType,
                               &demux->videoFormat.codec_data,
                               demux->videoFormat.codecID);
    }

    SafeRelease(&pMediaType);

    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

static gboolean mfdemux_configure_video_src_caps(GstMFDemux *demux)
{
    gboolean ret = FALSE;
    GstCaps *caps = NULL;
    GstEvent *caps_event = NULL;

    if (demux->videoFormat.codecID != JFX_CODEC_ID_H264 &&
        demux->videoFormat.codecID != JFX_CODEC_ID_HEVC)
        return FALSE; // We should not be called with unsupported codec

    if (demux->videoFormat.codecID == JFX_CODEC_ID_H264)
    {
        // Do not set width and height for H.264. In this case our
        // DirectShow wrapper will consider this format as H264 with start codes.
        // With width and height set DirectShow will try to decode it as H.264
        // without start codes and it will fail. Once we switched to
        // MediaFoundation for H.264 decoding we should set width and height
        // similar to HEVC.
        caps = gst_caps_new_simple ("video/x-h264",
                //"width", G_TYPE_INT, (gint)demux->videoFormat.uiWidth,
                //"height", G_TYPE_INT, (gint)demux->videoFormat.uiHeight,
                NULL);
    }
    else if (demux->videoFormat.codecID == JFX_CODEC_ID_HEVC)
    {
        caps = gst_caps_new_simple ("video/x-h265",
                "width", G_TYPE_INT, (gint)demux->videoFormat.uiWidth,
                "height", G_TYPE_INT, (gint)demux->videoFormat.uiHeight,
                NULL);
    }

    if (caps == NULL)
        return FALSE;

    if (!demux->pGSTMFByteStream->IsSeekSupported())
    {
        gst_caps_set_simple(caps, "fragmented", G_TYPE_BOOLEAN,
                            TRUE, NULL);
    }

    if (demux->videoFormat.codec_data)
        gst_caps_set_simple(caps, "codec_data", GST_TYPE_BUFFER,
                            demux->videoFormat.codec_data, NULL);

    caps_event = gst_event_new_caps(caps);
    if (caps_event)
        ret = gst_pad_push_event(demux->video_src_pad, caps_event);
    gst_caps_unref(caps);

    return ret;
}

static gboolean mfdemux_configure_video_src_pad(GstMFDemux *demux)
{
    if (!demux)
        return FALSE;

    if (demux->video_src_pad)
        return TRUE;

    if (demux->videoFormat.codecID != JFX_CODEC_ID_H264 &&
        demux->videoFormat.codecID != JFX_CODEC_ID_HEVC)
    {
        return TRUE; // Just ignore unknown video stream
    }

    demux->video_src_pad =
            gst_pad_new_from_template(gst_element_class_get_pad_template
            (GST_ELEMENT_GET_CLASS(demux), "video"), "video");
    if (demux->video_src_pad == NULL)
        return FALSE;

    gst_pad_set_query_function(demux->video_src_pad, mfdemux_src_query);
    gst_pad_set_event_function(demux->video_src_pad, mfdemux_src_event);

    if (!gst_pad_set_active(demux->video_src_pad, TRUE) ||
        !mfdemux_configure_video_src_caps(demux))
    {
        gst_object_unref(demux->video_src_pad);
        demux->video_src_pad = NULL;
        return FALSE;
    }

    gst_pad_use_fixed_caps(demux->video_src_pad);

    if (!gst_element_add_pad(GST_ELEMENT(demux), demux->video_src_pad)) {
        // Pad will be unref even if gst_element_add_pad() fails
        demux->video_src_pad = NULL;
        return FALSE;
    }

    return TRUE;
}

// Enables streams and creates src pads
static gboolean mfdemux_configure_demux(GstMFDemux *demux)
{
    gboolean hasAudio = false;
    gboolean hasVideo = false;

    if (!demux->is_demux_initialized)
        return FALSE;

    if (!mfdemux_configure_audio_stream(demux, &hasAudio))
        return FALSE;

    if (hasAudio && !mfdemux_configure_audio_src_pad(demux))
        return FALSE;

    if (!mfdemux_configure_video_stream(demux, &hasVideo))
        return FALSE;

    if (hasVideo && !mfdemux_configure_video_src_pad(demux))
        return FALSE;

    // No more pads are expected
    gst_element_no_more_pads(GST_ELEMENT(demux));

    return TRUE;
}

static void mfdemux_send_new_segment(GstMFDemux *demux, GstClockTime position)
{
    GstSegment segment;
    GstEvent *new_segment = NULL;

    gst_segment_init(&segment, GST_FORMAT_TIME);

    segment.rate = demux->rate;
    segment.start = demux->seek_position;
    segment.stop = demux->llDuration * 100;
    segment.time = demux->seek_position;
    segment.position = position;
    segment.duration = demux->llDuration * 100;

    new_segment = gst_event_new_segment(&segment);
    mfdemux_push_sink_event(demux, new_segment);
}

static GstFlowReturn mfdemux_deliver_sample(GstMFDemux *demux, GstPad* pad,
                                            IMFSample *pMFSample)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    IMFMediaBuffer *pMFBuffer = NULL;
    gboolean unlock_buffer = FALSE;
    BYTE *pbMFBuffer = NULL;
    DWORD cbMFCurrentLength = 0;
    GstBuffer *pBuffer = NULL;
    GstMapInfo info;
    gboolean unmap_buffer = FALSE;

    // Allocate GStreamer buffer and copy data to it
    HRESULT hr = pMFSample->ConvertToContiguousBuffer(&pMFBuffer);
    if (SUCCEEDED(hr))
        hr = pMFBuffer->Lock(&pbMFBuffer, NULL, &cbMFCurrentLength);

    if (SUCCEEDED(hr))
        unlock_buffer = TRUE;

    if (SUCCEEDED(hr))
        pBuffer = gst_buffer_new_allocate(NULL, (gsize)cbMFCurrentLength, NULL);

    if (pBuffer == NULL)
        hr = E_POINTER;

    if (SUCCEEDED(hr) && !gst_buffer_map(pBuffer, &info, GST_MAP_READWRITE))
        hr = E_FAIL;
    else
        unmap_buffer = TRUE;

    if (memcpy_s(info.data, info.maxsize, pbMFBuffer,cbMFCurrentLength) != 0)
        hr = E_FAIL;

    if (unmap_buffer)
        gst_buffer_unmap(pBuffer, &info);

    if (unlock_buffer)
        pMFBuffer->Unlock();

    SafeRelease(&pMFBuffer);

    // Set PTS, duration, etc.
    LONGLONG hnsSampleTime = -1;
    if (SUCCEEDED(hr) && SUCCEEDED(pMFSample->GetSampleTime(&hnsSampleTime)))
        GST_BUFFER_TIMESTAMP(pBuffer) = (hnsSampleTime * 100);

    LONGLONG hnsSampleDuration = -1;
    if (SUCCEEDED(hr) && SUCCEEDED(pMFSample->GetSampleDuration(&hnsSampleDuration)))
        GST_BUFFER_DURATION(pBuffer) = (hnsSampleDuration * 100);

    UINT32 bDiscontinuity = FALSE;
    if (SUCCEEDED(hr) && SUCCEEDED(pMFSample->GetUINT32(MFSampleExtension_Discontinuity, &bDiscontinuity)))
    {
        if (bDiscontinuity)
            GST_BUFFER_FLAG_SET(pBuffer, GST_BUFFER_FLAG_DISCONT);
    }

    // Before pushing buffer send new segment if needed
    if (demux->send_new_segment)
    {
        mfdemux_send_new_segment(demux, GST_BUFFER_TIMESTAMP(pBuffer));
        demux->send_new_segment = FALSE;
    }
    else if (demux->cached_segment_event != NULL)
    {
        mfdemux_push_sink_event(demux, demux->cached_segment_event);
        demux->cached_segment_event = NULL;
    }

    if (SUCCEEDED(hr))
        ret = gst_pad_push(pad, pBuffer);

    if (FAILED(hr))
    {
        // Since we did not push buffer, unref it if needed.
        if (pBuffer != NULL)
        {
            // INLINE - gst_buffer_unref()
            gst_buffer_unref(pBuffer);
        }
    }

    return ret;
}

static GstPad* mfdemux_get_src_pad(GstMFDemux *demux, DWORD index)
{
    if (demux->audio_stream_index == index)
        return demux->audio_src_pad;
    else if (demux->video_stream_index == index)
        return demux->video_src_pad;

    // We probbaly do not know yet index -> src_pad mapping.
    IMFMediaType *pMediaType = NULL;
    HRESULT hr = demux->pSourceReader->GetCurrentMediaType(index, &pMediaType);
    if (SUCCEEDED(hr) && pMediaType != NULL)
    {
        GUID guidMajorType;
        hr = pMediaType->GetMajorType(&guidMajorType);
        SafeRelease(&pMediaType);
        if (SUCCEEDED(hr) && IsEqualGUID(guidMajorType, MFMediaType_Audio))
        {
            demux->audio_stream_index = index;
            return demux->audio_src_pad;
        }
        else if (SUCCEEDED(hr) && IsEqualGUID(guidMajorType, MFMediaType_Video))
        {
            demux->video_stream_index = index;
            return demux->video_src_pad;
        }
    }

    return NULL;
}

static void mfdemux_loop(GstPad * pad)
{
    GstMFDemux *demux = GST_MFDEMUX(GST_PAD_PARENT(pad));
    GstFlowReturn result = GST_FLOW_OK;

    if (!demux->is_demux_initialized)
    {
        GST_PAD_STREAM_UNLOCK(pad);
        // TODO post fatal error
        if (mfdemux_init_demux(demux, NULL))
        {
            mfdemux_configure_demux(demux);
        }
        GST_PAD_STREAM_LOCK(pad);
    }

    g_mutex_lock(&demux->lock);
    result = demux->src_result;
    g_mutex_unlock(&demux->lock);

    if (result != GST_FLOW_OK)
    {
        gst_pad_pause_task(pad);
        return;
    }

    if (demux->pSourceReader == NULL)
    {
        gst_pad_pause_task(pad);
        return;
    }

    DWORD dwActualStreamIndex = 0;
    DWORD dwStreamFlags = 0;
    LONGLONG llTimestamp = -1;
    IMFSample *pSample = NULL;
    GST_PAD_STREAM_UNLOCK(pad);
    HRESULT hr = demux->pSourceReader->ReadSample(MF_SOURCE_READER_ANY_STREAM,
                                                0,
                                                &dwActualStreamIndex,
                                                &dwStreamFlags,
                                                &llTimestamp,
                                                &pSample);
    GST_PAD_STREAM_LOCK(pad);
    if (hr == S_OK && pSample != NULL)
    {
        GstPad *src_pad = mfdemux_get_src_pad(demux, dwActualStreamIndex);
        if (src_pad != NULL)
            result = mfdemux_deliver_sample(demux, src_pad, pSample);
        if (result != GST_FLOW_OK)
        {
            g_print("AMDEBUG mfdemux_deliver_sample() failed with %d\n", result);
        }

        SafeRelease(&pSample);
    }
    else if (hr == S_OK)
    {
        if ((dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) == MF_SOURCE_READERF_ENDOFSTREAM)
        {
            // Deliver EOS to all src pads, since source reader reports it for
            // last read only and not for each stream.
            mfdemux_push_sink_event(demux, gst_event_new_eos());
            result = GST_FLOW_EOS;
            //result = GST_FLOW_OK;
        }
    }
    else
    {
        gst_element_message_full(GST_ELEMENT(demux), GST_MESSAGE_ERROR,
            GST_STREAM_ERROR, GST_STREAM_ERROR_DEMUX,
            g_strdup_printf("ReadSample() failed (0x%X)", hr), NULL,
            ("mfdemux.c"), ("mfdemux_loop"), 0);
        result = GST_FLOW_ERROR;
    }

    g_mutex_lock(&demux->lock);
    if (GST_FLOW_OK == demux->src_result || GST_FLOW_OK != result)
        demux->src_result = result;
    else
        result = demux->src_result;
    g_mutex_unlock(&demux->lock);

    if (result != GST_FLOW_OK)
        gst_pad_pause_task(pad);
}

static gboolean mfdemux_activate(GstPad *pad, GstObject *parent)
{
    //GstQuery *query = NULL;
    //gboolean pull_mode = FALSE;

    //// First check what upstream scheduling is supported
    //query = gst_query_new_scheduling ();
    //if (!gst_pad_peer_query (pad, query))
    //{
    //    gst_query_unref (query);
    //    goto activate_push;
    //}

    //// Check if pull-mode is supported
    //pull_mode = gst_query_has_scheduling_mode_with_flags (query,
    //        GST_PAD_MODE_PULL, GST_SCHEDULING_FLAG_SEEKABLE);
    //gst_query_unref (query);

    //if (!pull_mode)
    //    goto activate_push;

    // Activate pull mode
    return gst_pad_activate_mode (pad, GST_PAD_MODE_PULL, TRUE);

activate_push:
    {
        // Fallback to push-mode. Not supported and chain() will return error.
        return gst_pad_activate_mode (pad, GST_PAD_MODE_PUSH, TRUE);
    }
}

static gboolean mfdemux_activate_mode(GstPad *pad, GstObject *parent, GstPadMode mode, gboolean active)
{
    GstMFDemux *demux = GST_MFDEMUX(parent);
    gboolean res = FALSE;

    switch (mode)
    {
    case GST_PAD_MODE_PUSH:
        res = TRUE;
        break;
    case GST_PAD_MODE_PULL:
        if (active)
        {
            g_mutex_lock(&demux->lock);
            demux->src_result = GST_FLOW_OK;
            g_mutex_unlock(&demux->lock);

            res = gst_pad_start_task(pad, (GstTaskFunction) mfdemux_loop,
                    pad, NULL);
        }
        else
        {
            g_mutex_lock(&demux->lock);
            demux->src_result = GST_FLOW_ERROR;
            g_mutex_unlock(&demux->lock);

            // Lock pad. Streaming thread might be waiting for data, but
            // it should release stream lock when doing it.
            GST_PAD_STREAM_LOCK(demux->sink_pad);
            // Unblock source reader if it was waiting for read.
            if (demux->pGSTMFByteStream)
                demux->pGSTMFByteStream->CompleteReadData(S_OK);
            // Unlock stream lock so streaming thread can continue.
            GST_PAD_STREAM_UNLOCK(demux->sink_pad);

            res = gst_pad_stop_task(pad);
        }
        break;
    default:
        // Unknown scheduling mode
        res = FALSE;
        break;
    }

    return res;
}

gboolean mfdemux_init(GstPlugin* mfdemux)
{
    return gst_element_register(mfdemux, "mfdemux", 512, GST_TYPE_MFDEMUX);
}
