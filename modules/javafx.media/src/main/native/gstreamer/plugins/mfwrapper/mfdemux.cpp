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

#include <mfdemux.h>

//#include <mfidl.h>
//#include <Wmcodecdsp.h>

using namespace std;

#define MAX_CODEC_DATA_SIZE 256
//#define PTS_DEBUG 0

// enum
// {
//     PROP_0,
//     PROP_CODEC_ID,
//     PROP_IS_SUPPORTED,
// };

// enum
// {
//     PO_DELIVERED,
//     PO_NEED_MORE_DATA,
//     PO_FLUSHING,
//     PO_FAILED,
// };

GST_DEBUG_CATEGORY_STATIC(gst_mfdemux_debug);
#define GST_CAT_DEFAULT gst_mfdemux_debug

// The input capabilities
static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        "video/mp4"
//        "video/mp4;"
//        "video/quicktime;"
//        "audio/x-m4a;"
//        "video/x-m4v"
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
    // demux->force_output_discontinuity = FALSE;

    demux->pGSTMFByteStream = NULL;
    demux->pIMFByteStream = NULL;
    demux->pSourceReader = NULL;

    // Init audio format for some defaults
    demux->audioFormat.codecID = JFX_CODEC_ID_UNKNOWN;
    demux->audioFormat.uiChannels = 2;
    demux->audioFormat.uiRate = 48000;
    demux->audioFormat.codec_data = NULL;

    demux->audio_src_pad = NULL;

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

    //if (gst_pad_is_linked(demux->srcpad))
    //     ret = gst_pad_push_event(demux->srcpad, gst_event_ref(event));  // INLINE - gst_event_ref()

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
        ret = mfdemux_push_sink_event(demux, event);
        demux->is_eos_received = FALSE;
        demux->is_eos = FALSE;
    }
    break;
    case GST_EVENT_FLUSH_START:
    {
        demux->is_flushing = TRUE;

        ret = mfdemux_push_sink_event(demux, event);
    }
    break;
    case GST_EVENT_FLUSH_STOP:
    {
        // demux->pDecoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
        // if (demux->pColorConvert)
        //     demux->pColorConvert->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);

        ret = mfdemux_push_sink_event(demux, event);

        demux->is_flushing = FALSE;
    }
    break;
    case GST_EVENT_EOS:
    {
        demux->is_eos_received = TRUE;

        // // Let demux know that we got end of stream
        // hr = demux->pDecoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);

        // // Ask demux to produce all remaining data
        // if (SUCCEEDED(hr))
        //     demux->pDecoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);

        // // Deliver remaining data
        // gint po_ret;
        // do
        // {
        //     po_ret = mfdemux_process_output(demux);
        // } while (po_ret == PO_DELIVERED);

        // if (demux->pColorConvert)
        // {
        //     hr = demux->pColorConvert->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        //     if (SUCCEEDED(hr))
        //         hr = demux->pColorConvert->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
        // }

        // We done pushing all frames. Deliver EOS.
        ret = mfdemux_push_sink_event(demux, event);

        demux->is_eos = TRUE;
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

        gst_event_unref(event);
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
            if (format != GST_FORMAT_TIME)
            {
                ret = gst_pad_query_default(pad, parent, query);
            }
            else
            {
                //gst_query_set_duration(query, GST_FORMAT_TIME, filter->metadata->duration);
            }
        }
        break;
        default:
        {
            ret = gst_pad_query_default(pad, parent, query);
        }
    }

    return ret;
}

static gboolean mfdemux_src_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    gboolean ret = TRUE;
    GstMFDemux *demux = GST_MFDEMUX(parent);

    switch (GST_EVENT_TYPE (event))
    {
        case GST_EVENT_SEEK:
            // TODO Seek
            // INLINE - gst_event_unref()
            gst_event_unref (event);
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

    demux->pGSTMFByteStream = new (nothrow) CGSTMFByteStream((QWORD)data_length, demux->sink_pad);
    if (demux->pGSTMFByteStream == NULL)
        return FALSE;

    hr = demux->pGSTMFByteStream->QueryInterface(IID_IMFByteStream, (void**)&demux->pIMFByteStream);
    if (FAILED(hr) || demux->pIMFByteStream == NULL)
        return FALSE;

    hr = MFCreateSourceReaderFromByteStream(demux->pIMFByteStream, NULL, &demux->pSourceReader);
    if (FAILED(hr) || demux->pSourceReader == NULL)
        return FALSE;

    // Disable all streams. Disabled streams does not consume memory if not
    // read. MP4 might contain subtitles or additional audio stream and we do
    // not support it. We will enable needed streams when configuring demux.
    hr = demux->pSourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    if (FAILED(hr))
        return FALSE;

    demux->is_demux_initialized = TRUE;

    return TRUE;
}

static UINT32 mfdemux_adjust_codec_data_blob()
// If codec_data not available or fail to read, then codec_data will be set to NULL.
// We will attempt to playback anyway.
static void mfdemux_get_codec_data(const GUID &guidKey, IMFMediaType *pMediaType, GstBuffer **codec_data)
{
    UINT32 cbBlobSize = 0;
    UINT8 blobBytes[MAX_CODEC_DATA_SIZE] = {0};

    if (pMediaType == NULL)
        return;

    HRESULT hr = pMediaType->GetBlobSize(guidKey, &cbBlobSize);
    if (SUCCEEDED(hr) && cbBlobSize > 0 && cbBlobSize <= MAX_CODEC_DATA_SIZE)
    {
        hr = pMediaType->GetBlob(guidKey, &blobBytes[0], cbBlobSize, NULL);
        if (SUCCEEDED(hr))

        (*codec_data) = gst_buffer_new_allocate(NULL, (gsize)cbBlobSize, NULL);
        if ((*codec_data) != NULL)
        {
            GstMapInfo info;
            if (gst_buffer_map((*codec_data), &info, GST_MAP_READWRITE))
            {
                hr = pMediaType->GetBlob(guidKey, (UINT8*)info.data, cbBlobSize, NULL);
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

static gboolean mfdemux_configure_audio_stream(GstMFDemux *demux)
{
    HRESULT hr = S_OK;
    IMFMediaType *pMediaType = NULL;
    GUID subType;

    hr = demux->pSourceReader->
        SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

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
        mfdemux_get_codec_data(MF_MT_USER_DATA, pMediaType,
                &demux->audioFormat.codec_data);
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

// Enables streams and creates src pads
static gboolean mfdemux_configure_demux(GstMFDemux *demux)
{
    if (!demux->is_demux_initialized)
        return FALSE;

    if (!mfdemux_configure_audio_stream(demux))
        return FALSE;

    if (!mfdemux_configure_audio_src_pad(demux))
        return FALSE;

    // No more pads are expected
    gst_element_no_more_pads(GST_ELEMENT(demux));

    return TRUE;
}

static GstFlowReturn mfdemux_deliver_sample(GstPad* pad, IMFSample *pMFSample)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    IMFMediaBuffer *pMFBuffer = NULL;
    gboolean unlock_buffer = FALSE;
    BYTE *pbMFBuffer = NULL;
    DWORD cbMFCurrentLength = 0;
    GstBuffer *pBuffer = NULL;
    GstMapInfo info;
    gboolean unmap_buffer = FALSE;

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

static void mfdemux_loop(GstPad * pad)
{
    GstMFDemux *demux = GST_MFDEMUX(GST_PAD_PARENT(pad));

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

    if (demux->pSourceReader == NULL)
    {
        gst_pad_pause_task(pad);
        return;
    }

    DWORD dwActualStreamIndex = 0;
    DWORD dwStreamFlags = 0;
    LONGLONG llTimestamp = -1;
    IMFSample *pSample = NULL;
    HRESULT hr = demux->pSourceReader->ReadSample(MF_SOURCE_READER_ANY_STREAM,
                                                0,
                                                &dwActualStreamIndex,
                                                &dwStreamFlags,
                                                &llTimestamp,
                                                &pSample);
    if (hr == S_OK)
    {
        GstPad *src_pad = NULL;
        //if (dwActualStreamIndex == MF_SOURCE_READER_FIRST_AUDIO_STREAM)
            src_pad = demux->audio_src_pad;

        if (src_pad != NULL)
            mfdemux_deliver_sample(src_pad, pSample);
    }

    SafeRelease(&pSample);


    //gst_pad_pause_task(pad);
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
    gboolean res = FALSE;

    switch (mode) {
    case GST_PAD_MODE_PUSH:
        res = TRUE;
        break;
    case GST_PAD_MODE_PULL:
        if (active) {
            res = gst_pad_start_task (pad, (GstTaskFunction) mfdemux_loop,
                                      pad, NULL);
        } else {
            res = gst_pad_stop_task (pad);
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
