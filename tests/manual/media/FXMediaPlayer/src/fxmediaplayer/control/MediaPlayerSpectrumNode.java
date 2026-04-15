/*
 * Copyright (c) 2021, Oracle and/or its affiliates. All rights reserved.
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

package fxmediaplayer.control;

import fxmediaplayer.FXMediaPlayerControlInterface;
import fxmediaplayer.FXMediaPlayerInterface;
import javafx.beans.InvalidationListener;
import javafx.beans.Observable;
import javafx.beans.property.ReadOnlyObjectProperty;
import javafx.event.Event;
import javafx.scene.Node;
import javafx.scene.chart.BarChart;
import javafx.scene.chart.CategoryAxis;
import javafx.scene.chart.Chart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.XYChart;
import javafx.scene.control.Tab;
import javafx.scene.layout.VBox;
import javafx.scene.media.AudioSpectrumListener;
import javafx.scene.media.MediaPlayer;

public class MediaPlayerSpectrumNode implements FXMediaPlayerControlInterface {

    private FXMediaPlayerInterface FXMediaPlayer = null;
    private VBox spectrum = null;
    private AudioSpectrumListenerImpl audioSpectrumListener = null;
    private XYChart.Data<String, Number>[] seriesData;

    public MediaPlayerSpectrumNode(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;
    }

    public Node getNode() {
        if (spectrum == null) {
            spectrum = new VBox();

            spectrum.getChildren().add(createBarChart());
        }

        return spectrum;
    }

    @Override
    public void onMediaPlayerChanged(MediaPlayer oldMediaPlayer) {
        removeListeners(oldMediaPlayer);
        addListeners();
    }

    @SuppressWarnings("unchecked")
    private void addListeners() {
        if (FXMediaPlayer.getMediaPlayer() == null) {
            return;
        }

        if (audioSpectrumListener == null) {
            audioSpectrumListener = new AudioSpectrumListenerImpl();
        }

        FXMediaPlayer.getMediaPlayer().setAudioSpectrumListener(audioSpectrumListener);
    }

    private void removeListeners(MediaPlayer mediaPlayer) {
        if (mediaPlayer == null) {
            return;
        }

        mediaPlayer.setAudioSpectrumListener(null);
    }

    @SuppressWarnings("unchecked")
    private Chart createBarChart() {
        final CategoryAxis xAxis = new CategoryAxis();
        final NumberAxis yAxis = new NumberAxis(0, 50, 10);
        final BarChart<String, Number> bc = new BarChart<>(xAxis, yAxis);

        bc.setId("mediaPlayerSpectrum");
        bc.setLegendVisible(false);
        bc.setAnimated(false);
        bc.setBarGap(0);
        bc.setCategoryGap(1);
        bc.setVerticalGridLinesVisible(false);
        yAxis.setTickLabelFormatter(new NumberAxis.DefaultFormatter(yAxis, null, "dB"));
        XYChart.Series<String, Number> series = new XYChart.Series<>();
        series.setName("Data Series 1");
        seriesData = new XYChart.Data[128];
        String[] categories = new String[128];
        for (int i = 0; i < seriesData.length; i++) {
            categories[i] = Integer.toString(i + 1);
            seriesData[i] = new XYChart.Data<>(categories[i], 50);
            series.getData().add(seriesData[i]);
        }
        bc.getData().add(series);

        return bc;
    }

    private class AudioSpectrumListenerImpl implements AudioSpectrumListener {

        @Override
        public void spectrumDataUpdate(double timestamp, double duration,
                float[] magnitudes, float[] phases) {
            for (int i = 0; i < seriesData.length; i++) {
                seriesData[i].setYValue(magnitudes[i] + 60);
            }
        }
    }
}
