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

import fxmediaplayer.FXMediaPlayerInterface;
import fxmediaplayer.test.MediaPlayerController;
import javafx.application.Platform;
import javafx.beans.InvalidationListener;
import javafx.beans.Observable;
import javafx.beans.property.DoubleProperty;
import javafx.beans.property.IntegerProperty;
import javafx.beans.property.ReadOnlyObjectProperty;
import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.event.ActionEvent;
import javafx.scene.control.Button;
import javafx.scene.control.Slider;
import javafx.scene.control.ToggleButton;
import javafx.scene.control.ToolBar;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.VBox;
import javafx.scene.media.MediaPlayer;
import javafx.scene.input.MouseEvent;
import javafx.util.Duration;

public class MediaPlayerToolBar {

    private FXMediaPlayerInterface FXMediaPlayer = null;
    private VBox vbox = null;
    private ToolBar toolBar = null;
    private ToolBar toolBarTimeSlider = null;
    private Button buttonPlay = null;
    private Button buttonPause = null;
    private Button buttonStop = null;
    private ToggleButton buttonMute = null;
    private ToggleButton buttonSmooth = null;
    private ToggleButton buttonPreserveRatio = null;
    private ToggleButton buttonLoop = null;
    private Button buttonStartTime = null;
    private Button buttonStopTime = null;
    private Button buttonCycleCount = null;
    private Button buttonAddMarker = null;
    private final Slider timeSlider = new Slider();
    private boolean disableTimeSliderUpdate = false;
    private Duration duration = null;
    private InvalidationListener durationPropertyListener = null;
    private ChangeListener<Duration> currentTimePropertyListener = null;
    private InvalidationListener statusPropertyListener = null;

    public MediaPlayerToolBar(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;
    }

    public VBox getToolBar() {
        if (vbox == null) {
            vbox = new VBox();

            toolBarTimeSlider = new ToolBar();

            timeSlider.setMinWidth(50);
            HBox.setHgrow(timeSlider, Priority.ALWAYS);
            timeSlider.setMaxWidth(Double.MAX_VALUE);
            timeSlider.setOnMousePressed((MouseEvent me) -> {
                onTimeSliderPressed();
            });
            timeSlider.setOnMouseReleased((MouseEvent me) -> {
                onTimeSliderReleased();
            });
            timeSlider.valueProperty().addListener(
                    (ObservableValue<? extends Number> ov, Number o, Number n) -> {
                onTimeSlider();
            });
            timeSlider.setDisable(true);

            toolBarTimeSlider.getItems().add(timeSlider);

            toolBarTimeSlider.setDisable(true);

            vbox.getChildren().add(toolBarTimeSlider);

            toolBar = new ToolBar();

            // Play
            buttonPlay = new Button("Play");
            buttonPlay.setOnAction((ActionEvent event) -> {
                onButtonPlay();
            });
            toolBar.getItems().add(buttonPlay);

            // Pause
            buttonPause = new Button("Pause");
            buttonPause.setOnAction((ActionEvent event) -> {
                onButtonPause();
            });
            toolBar.getItems().add(buttonPause);

            // Stop
            buttonStop = new Button("Stop");
            buttonStop.setOnAction((ActionEvent event) -> {
                onButtonStop();
            });
            toolBar.getItems().add(buttonStop);

            // Mute
            buttonMute = new ToggleButton("Mute");
            buttonMute.setOnAction((ActionEvent event) -> {
                onButtonMute();
            });
            toolBar.getItems().add(buttonMute);

            // Smooth
            buttonSmooth = new ToggleButton("Smooth");
            buttonSmooth.setOnAction((ActionEvent event) -> {
                onButtonSmooth();
            });
            if (FXMediaPlayer.getMediaView().isSmooth()) {
                buttonSmooth.setSelected(true);
            }
            toolBar.getItems().add(buttonSmooth);

            // Ratio
            buttonPreserveRatio = new ToggleButton("Ratio");
            buttonPreserveRatio.setOnAction((ActionEvent event) -> {
                onButtonPreserveRatio();
            });
            if (FXMediaPlayer.getMediaView().isPreserveRatio()) {
                buttonPreserveRatio.setSelected(true);
            }
            toolBar.getItems().add(buttonPreserveRatio);

            // Loop
            buttonLoop = new ToggleButton("Loop");
            buttonLoop.setOnAction((ActionEvent event) -> {
                onButtonLoop();
            });
            toolBar.getItems().add(buttonLoop);

            // Start Time
            buttonStartTime = new Button("Start Time");
            buttonStartTime.setOnAction((ActionEvent event) -> {
                onButtonStartTime();
            });
            toolBar.getItems().add(buttonStartTime);

            // Stop Time
            buttonStopTime = new Button("Stop Time");
            buttonStopTime.setOnAction((ActionEvent event) -> {
                onButtonStopTime();
            });
            toolBar.getItems().add(buttonStopTime);

            // Cycle Count
            buttonCycleCount = new Button("Cycle Count");
            buttonCycleCount.setOnAction((ActionEvent event) -> {
                onButtonCycleCount();
            });
            toolBar.getItems().add(buttonCycleCount);

            // Add Marker
            buttonAddMarker = new Button("Add Marker");
            buttonAddMarker.setOnAction((ActionEvent event) -> {
                onButtonAddMarker();
            });
            toolBar.getItems().add(buttonAddMarker);

            toolBar.setDisable(true);

            vbox.getChildren().add(toolBar);

            addToController();
            createListeners();
            addListeners();
        }

        return vbox;
    }

    public void onMediaPlayerChanged(MediaPlayer oldMediaPlayer) {
        removeListeners(oldMediaPlayer);
        addListeners();
        onButtonMute();
        onButtonSmooth();
        onButtonPreserveRatio();
        onButtonLoop();
    }

    private void addToController() {
        if (FXMediaPlayer.getTester() == null) {
            return;
        }

        if (FXMediaPlayer.getTester().getController() == null) {
            return;
        }

        MediaPlayerController controller = FXMediaPlayer.getTester().getController();
        controller.setPlayButton(buttonPlay);
        controller.setPauseButton(buttonPause);
        controller.setStopButton(buttonStop);
    }

    @SuppressWarnings("unchecked")
    private void createListeners() {
        durationPropertyListener = (Observable o) -> {
            ReadOnlyObjectProperty property = (ReadOnlyObjectProperty) o;
            Duration d = ((Duration) property.getValue());
            if (d.isIndefinite()) {
                timeSlider.setDisable(true);
            } else {
                if (d.toMillis() > 0) {
                    if (duration == null || !duration.equals(duration)) {
                        duration = d;
                        timeSlider.setDisable(false);
                    }
                }
            }
        };

        currentTimePropertyListener =
                (ObservableValue<? extends Duration> ov, Duration o, Duration n) -> {
            if (duration != null) {
                final Duration currentTime = n;
                Platform.runLater(() -> {
                    synchronized (timeSlider) {
                        if (!disableTimeSliderUpdate) {
                            if (duration != null) {
                                timeSlider.setValue(currentTime.divide(duration.toMillis()).toMillis() * 100.0);
                            } else {
                                timeSlider.setValue(0.0);
                            }
                        }
                    }
                });
            }
        };

        statusPropertyListener = (Observable o) -> {
            onStatus(o);
        };
    }

    private void addListeners() {
        if (FXMediaPlayer.getMediaPlayer() == null) {
            return;
        }

        FXMediaPlayer.getMediaPlayer().getMedia().durationProperty()
                    .addListener(durationPropertyListener);
        FXMediaPlayer.getMediaPlayer().currentTimeProperty()
                    .addListener(currentTimePropertyListener);
        FXMediaPlayer.getMediaPlayer()
                .statusProperty().addListener(statusPropertyListener);
    }

    private void removeListeners(MediaPlayer mediaPlayer) {
        if (mediaPlayer == null) {
            return;
        }

        mediaPlayer.getMedia().durationProperty()
                .removeListener(durationPropertyListener);
        mediaPlayer.currentTimeProperty()
                .removeListener(currentTimePropertyListener);
        mediaPlayer.statusProperty()
                .removeListener(statusPropertyListener);
    }

    @SuppressWarnings("unchecked")
    private void onStatus(Observable o) {
        try {
            ReadOnlyObjectProperty<MediaPlayer.Status> prop =
                    (ReadOnlyObjectProperty<MediaPlayer.Status>) o;
            MediaPlayer.Status status = prop.getValue();
            if (status == MediaPlayer.Status.READY) {
                toolBar.setDisable(false);
                toolBarTimeSlider.setDisable(false);
            } else if (status == MediaPlayer.Status.DISPOSED ||
                    status == MediaPlayer.Status.HALTED) {
                toolBar.setDisable(true);
                toolBarTimeSlider.setDisable(true);
            }
        } catch (Exception e) {
            System.err.println(e.toString());
        }
    }

    private void onButtonPlay() {
        if (FXMediaPlayer.getMediaPlayer() != null) {
            FXMediaPlayer.getMediaPlayer().play();
        }
    }

    private void onButtonPause() {
        if (FXMediaPlayer.getMediaPlayer() != null) {
            FXMediaPlayer.getMediaPlayer().pause();
        }
    }

    private void onButtonStop() {
        if (FXMediaPlayer.getMediaPlayer() != null) {
            FXMediaPlayer.getMediaPlayer().stop();
        }
    }

    private void onButtonMute() {
        if (FXMediaPlayer.getMediaPlayer() != null) {
            FXMediaPlayer.getMediaPlayer().setMute(buttonMute.isSelected());
        }
    }

    private void onButtonSmooth() {
        if (FXMediaPlayer.getMediaView() != null) {
            FXMediaPlayer.getMediaView().setSmooth(buttonSmooth.isSelected());
        }
    }

    private void onButtonPreserveRatio() {
        if (FXMediaPlayer.getMediaView() != null) {
            FXMediaPlayer.getMediaView().setPreserveRatio(buttonPreserveRatio.isSelected());
        }
    }

    private void onButtonLoop() {
        if (FXMediaPlayer.getMediaPlayer() != null) {
            if (buttonLoop.isSelected()) {
                FXMediaPlayer.getMediaPlayer().setCycleCount(MediaPlayer.INDEFINITE);
            } else {
                FXMediaPlayer.getMediaPlayer().setCycleCount(1);
            }
        }
    }

    private void onButtonStartTime() {
    }

    private void onButtonStopTime() {
    }

    private void onButtonCycleCount() {
    }

    private void onButtonAddMarker() {
    }

    private void onTimeSliderPressed() {
        synchronized (timeSlider) {
            disableTimeSliderUpdate = true;
        }
    }

    private void onTimeSliderReleased() {
        synchronized (timeSlider) {
            if (!FXMediaPlayer.getScrubbing()) {
                FXMediaPlayer.getMediaPlayer()
                        .seek(duration.multiply(timeSlider.getValue() / 100.0));
            }
            disableTimeSliderUpdate = false;
        }
    }

    private void onTimeSlider() {
        if (FXMediaPlayer.getScrubbing()) {
            if (timeSlider.isValueChanging()) {
                FXMediaPlayer.getMediaPlayer()
                        .seek(duration.multiply(timeSlider.getValue() / 100.0));
            }
        }
    }
}
