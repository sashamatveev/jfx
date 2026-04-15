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
import javafx.application.Platform;
import javafx.beans.InvalidationListener;
import javafx.beans.Observable;
import javafx.beans.property.DoubleProperty;
import javafx.beans.property.IntegerProperty;
import javafx.beans.property.ReadOnlyObjectProperty;
import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.event.ActionEvent;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.Button;
import javafx.scene.control.ContentDisplay;
import javafx.scene.control.Label;
import javafx.scene.control.Slider;
import javafx.scene.control.Tab;
import javafx.scene.control.TextField;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import javafx.scene.media.MediaPlayer;
import javafx.util.Duration;

public class MediaPlayerControlNode implements FXMediaPlayerControlInterface {

    private FXMediaPlayerInterface FXMediaPlayer = null;
    private VBox control = null;
    private Slider volumeSlider = null;
    private Slider balanceSlider = null;
    private Slider rateSlider = null;
    private Button buttonResetSlider = null;
    private InvalidationListener volumePropertyListener = null;
    private InvalidationListener balancePropertyListener = null;
    private InvalidationListener ratePropertyListener = null;

    public MediaPlayerControlNode(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;
    }

    public Node getNode() {
        if (control == null) {

            control = new VBox(15);

            // Volume
            volumeSlider = new Slider(0, 100, 1);
            volumeSlider.setValue(100);
            volumeSlider.setPrefWidth(70);
            volumeSlider.setMaxWidth(Region.USE_PREF_SIZE);
            volumeSlider.setMinWidth(30);
            volumeSlider.valueProperty().addListener(
                    (ObservableValue<? extends Number> ov, Number o, Number n) -> {
                onVolumeSlider();
            });
            Label label = new Label("Volume:", volumeSlider);
            label.setContentDisplay(ContentDisplay.RIGHT);
            control.getChildren().add(label);

            // Balance
            balanceSlider = new Slider(-100, 100, 1);
            balanceSlider.setValue(0);
            balanceSlider.setPrefWidth(140);
            balanceSlider.setMaxWidth(Region.USE_PREF_SIZE);
            balanceSlider.setMinWidth(60);
            balanceSlider.valueProperty().addListener(
                    (ObservableValue<? extends Number> ov, Number o, Number n) -> {
                onBalanceSlider();
            });
            label = new Label("Balance:", balanceSlider);
            label.setContentDisplay(ContentDisplay.RIGHT);
            control.getChildren().add(label);

            // Rate
            rateSlider = new Slider(0, 800, 1);
            rateSlider.setValue(100);
            rateSlider.setPrefWidth(140);
            rateSlider.setMaxWidth(Region.USE_PREF_SIZE);
            rateSlider.setMinWidth(60);
            rateSlider.setOnMouseReleased((MouseEvent me) -> {
                onRateSlider();
            });
            label = new Label("Rate:", rateSlider);
            label.setContentDisplay(ContentDisplay.RIGHT);
            control.getChildren().add(label);

            buttonResetSlider = new Button("Reset");
            buttonResetSlider.setOnAction((ActionEvent event) -> {
                onButtonResetSlider();
            });
            control.getChildren().add(buttonResetSlider);

            createListeners();
            addListeners();
        }

        return control;
    }

    @Override
    public void onMediaPlayerChanged(MediaPlayer oldMediaPlayer) {
        if (oldMediaPlayer != null) {
            removeListeners(oldMediaPlayer);
        }

        volumeSlider.setValue(100);
        balanceSlider.setValue(0);
        rateSlider.setValue(100);

        addListeners();
    }

    @SuppressWarnings("unchecked")
    private void createListeners() {
        volumePropertyListener = (Observable o) -> {
            DoubleProperty prop = (DoubleProperty) o;
            final double value = prop.getValue();
            Platform.runLater(() -> {
                volumeSlider.setValue(value * 100.0);
            });
        };

        balancePropertyListener = (Observable o) -> {
            DoubleProperty prop = (DoubleProperty) o;
            final double value = prop.getValue();
            Platform.runLater(() -> {
                balanceSlider.setValue(value * 100.0);
            });
        };

        ratePropertyListener = (Observable o) -> {
            DoubleProperty prop = (DoubleProperty) o;
            final double value = prop.getValue();
            Platform.runLater(() -> {
                rateSlider.setValue(value * 100.0);
            });
        };
    }

    private void addListeners() {
        MediaPlayer mediaPlayer = FXMediaPlayer.getMediaPlayer();
        if (mediaPlayer != null) {
            mediaPlayer.volumeProperty()
                    .addListener(volumePropertyListener);
            mediaPlayer.balanceProperty()
                    .addListener(balancePropertyListener);
            mediaPlayer.rateProperty()
                    .addListener(ratePropertyListener);
        }
    }

    private void removeListeners(MediaPlayer mediaPlayer) {
        mediaPlayer.volumeProperty()
                .removeListener(volumePropertyListener);
        mediaPlayer.balanceProperty()
                .removeListener(balancePropertyListener);
        mediaPlayer.rateProperty()
                .removeListener(ratePropertyListener);
    }

    private void onVolumeSlider() {
        if (volumeSlider.isValueChanging()) {
            FXMediaPlayer.getMediaPlayer()
                    .setVolume(volumeSlider.getValue() / 100.0);
        }
    }

    private void onBalanceSlider() {
        if (balanceSlider.isValueChanging()) {
            FXMediaPlayer.getMediaPlayer()
                    .setBalance(balanceSlider.getValue() / 100.0);
        }
    }

    private void onRateSlider() {
        FXMediaPlayer.getMediaPlayer().setRate(rateSlider.getValue() / 100.0);
    }

    private void onButtonResetSlider() {
        FXMediaPlayer.getMediaPlayer().setVolume(1.0);
        FXMediaPlayer.getMediaPlayer().setBalance(0.0);
        FXMediaPlayer.getMediaPlayer().setRate(1.0);
    }
}
