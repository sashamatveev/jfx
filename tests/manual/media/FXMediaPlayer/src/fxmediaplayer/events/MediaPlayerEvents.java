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

package fxmediaplayer.events;

import fxmediaplayer.FXMediaPlayerControlInterface;
import fxmediaplayer.FXMediaPlayerInterface;
import fxmediaplayer.FXMediaPlayerUtils;
import fxmediaplayer.test.MediaPlayerController;
import javafx.animation.FadeTransition;
import javafx.animation.SequentialTransition;
import javafx.application.Platform;
import javafx.beans.InvalidationListener;
import javafx.beans.Observable;
import javafx.beans.property.DoubleProperty;
import javafx.beans.property.ReadOnlyIntegerProperty;
import javafx.beans.property.ReadOnlyObjectProperty;
import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.ContentDisplay;
import javafx.scene.control.Label;
import javafx.scene.control.ListView;
import javafx.scene.control.ProgressIndicator;
import javafx.scene.layout.Priority;
import javafx.scene.layout.VBox;
import javafx.scene.media.MediaMarkerEvent;
import javafx.scene.media.MediaPlayer;
import javafx.scene.paint.Color;
import javafx.scene.shape.Rectangle;
import javafx.util.Duration;
import javafx.util.Pair;

public class MediaPlayerEvents implements FXMediaPlayerControlInterface {

    private static final double WIDTH = 210;
    private static final float ON_OPACITY = 1.0f;
    private static final float OFF_OPACITY = 0.1f;
    private FXMediaPlayerInterface FXMediaPlayer = null;
    private VBox events = null;
    private Label labelState = null;
    private ListView<String> mediaEvents = null;
    private final ObservableList<String> mediaEventsItems =
            FXCollections.observableArrayList();
    private Button buttonClearEvents = null;
    private InvalidationListener statusPropertyListener = null;
    private Runnable onReadyRunnable = null;
    private Runnable onPlayingRunnable = null;
    private Runnable onPausedRunnable = null;
    private Runnable onStoppedRunnable = null;
    private Runnable onStalledRunnable = null;
    private Runnable onHaltedRunnable = null;
    private Runnable onEndOfMediaRunnable = null;
    private Runnable onRepeatRunnable = null;
    private Runnable onErrorRunnable = null;
    private EventHandler<MediaMarkerEvent> onMarkerListener = null;

    public MediaPlayerEvents(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;
    }

    public VBox getEvents() {
        if (events == null) {
            events = new VBox(5);

            events.setMinWidth(WIDTH);
            events.setMaxWidth(WIDTH);
            events.setPrefWidth(WIDTH);
            events.setAlignment(Pos.TOP_CENTER);

            // Create state label
            labelState = createStateLabel(MediaPlayer.Status.UNKNOWN.toString(),
                    Color.LIGHTGRAY);
            events.getChildren().add(labelState);

            // Create event list view
            mediaEvents = new ListView<>();
            mediaEvents.setItems(mediaEventsItems);
            mediaEvents.setMaxHeight(Double.MAX_VALUE);
            mediaEvents.setMinWidth(WIDTH - 10);
            mediaEvents.setMaxWidth(WIDTH - 10);
            mediaEvents.setPrefWidth(WIDTH - 10);
            events.setVgrow(mediaEvents, Priority.ALWAYS);
            events.getChildren().add(mediaEvents);

            // Create clear button
            buttonClearEvents = new Button("Clear");
            buttonClearEvents.setOnAction((ActionEvent event) -> {
                onButtonClearEvents();
            });
            events.getChildren().add(buttonClearEvents);

            addToController();
            createListeners();
            addListeners();
        }

        return events;
    }

    @Override
    public void onMediaPlayerChanged(MediaPlayer oldMediaPlayer) {
        if (oldMediaPlayer != null) {
            removeListeners(oldMediaPlayer);
        }

        mediaEventsItems.clear();

        addListeners();
    }

    private void addToController() {
        if (FXMediaPlayer.getTester() == null) {
            return;
        }

        if (FXMediaPlayer.getTester().getController() == null) {
            return;
        }

        MediaPlayerController controller = FXMediaPlayer.getTester().getController();
        controller.setMediaEvents(mediaEventsItems);
        controller.setClearEvents(buttonClearEvents);
    }

    private void createListeners() {
        statusPropertyListener = (Observable o) -> {
            onStatus(o);
        };

        onReadyRunnable = () -> {
            onReady();
        };

        onPlayingRunnable = () -> {
            onPlaying();
        };

        onPausedRunnable = () -> {
            onPaused();
        };

        onStoppedRunnable = () -> {
            onStopped();
        };

        onStalledRunnable = () -> {
            onStalled();
        };

        onHaltedRunnable = () -> {
            onHalted();
        };

        onEndOfMediaRunnable = () -> {
            onEndOfMedia();
        };

        onRepeatRunnable = () -> {
            onRepeat();
        };

        onErrorRunnable = () -> {
            onError();
        };

        onMarkerListener = (MediaMarkerEvent event) -> {
            onMarker(event);
        };
    }

    private void addListeners() {
        if (FXMediaPlayer.getMediaPlayer() != null) {
            FXMediaPlayer.getMediaPlayer()
                    .statusProperty().addListener(statusPropertyListener);
            FXMediaPlayer.getMediaPlayer()
                    .setOnReady(onReadyRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnPlaying(onPlayingRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnPaused(onPausedRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnStopped(onStoppedRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnStalled(onStalledRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnHalted(onHaltedRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnEndOfMedia(onEndOfMediaRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnRepeat(onRepeatRunnable);
            FXMediaPlayer.getMediaPlayer()
                    .setOnError(onErrorRunnable);
            FXMediaPlayer.getMediaPlayer()
                .setOnMarker(onMarkerListener);
        }
    }

    private void removeListeners(MediaPlayer mediaPlayer) {
        mediaPlayer.statusProperty()
                .removeListener(statusPropertyListener);
        mediaPlayer.setOnReady(null);
        mediaPlayer.setOnPlaying(null);
        mediaPlayer.setOnPaused(null);
        mediaPlayer.setOnStopped(null);
        mediaPlayer.setOnStalled(null);
        mediaPlayer.setOnHalted(null);
        mediaPlayer.setOnEndOfMedia(null);
        mediaPlayer.setOnRepeat(null);
        mediaPlayer.setOnError(null);
        mediaPlayer.setOnMarker(null);
    }

    private void onButtonClearEvents() {
        mediaEventsItems.clear();
    }

    @SuppressWarnings("unchecked")
    private void onStatus(Observable o) {
        try {
            ReadOnlyObjectProperty<MediaPlayer.Status> prop =
                    (ReadOnlyObjectProperty<MediaPlayer.Status>) o;
            MediaPlayer.Status status = prop.getValue();
            if (status == MediaPlayer.Status.DISPOSED) {
                onDisposed();
            }
        } catch (Exception e) {
            System.err.println(e.toString());
        }
    }

    private void onReady() {
        logEvent(MediaPlayer.Status.READY.toString());
        switchState(MediaPlayer.Status.READY.toString());
    }

    private void onPlaying() {
        logEvent(MediaPlayer.Status.PLAYING.toString());
        switchState(MediaPlayer.Status.PLAYING.toString());
    }

    private void onPaused() {
        logEvent(MediaPlayer.Status.PAUSED.toString());
        switchState(MediaPlayer.Status.PAUSED.toString());
    }

    private void onStopped() {
        logEvent(MediaPlayer.Status.STOPPED.toString());
        switchState(MediaPlayer.Status.STOPPED.toString());
    }

    private void onStalled() {
        logEvent(MediaPlayer.Status.STALLED.toString());
        switchState(MediaPlayer.Status.STALLED.toString());
    }

    private void onDisposed() {
        logEvent(MediaPlayer.Status.DISPOSED.toString());
        switchState(MediaPlayer.Status.DISPOSED.toString());
    }

    private void onHalted() {
        logEvent(MediaPlayer.Status.HALTED.toString());
        switchState(MediaPlayer.Status.HALTED.toString());
    }

    private void onEndOfMedia() {
        logEvent("onEndOfMedia");
    }

    private void onRepeat() {
        logEvent("onRepeat");
    }

    private void onError() {
        System.err.println("Error: " + FXMediaPlayer.getMediaPlayer().getError().toString());
        logEvent("onError");
    }

    private void onMarker(MediaMarkerEvent event) {
        if (event != null) {
            Pair<String, Duration> pair = event.getMarker();
            logEvent(pair.getKey() + " (" + FXMediaPlayerUtils.secondsToString(pair.getValue().toSeconds()) + ")");
        };
    }

    private void logEvent(String event) {
        System.err.println("Event: " + event);
        String logEntry = mediaEventsItems.size() + ": " + event;
        mediaEventsItems.addFirst(logEntry);
    }

    private Label createStateLabel(String state, Color color) {
        Rectangle rect = new Rectangle(0, 0, 100, 30);
        rect.setArcHeight(20);
        rect.setArcWidth(20);
        rect.setFill(color);

        Label label = new Label(state, rect);
        label.setContentDisplay(ContentDisplay.CENTER);
        label.setOpacity(OFF_OPACITY);

        return label;
    }

    private void switchState(String text) {
        stateOFF(labelState);
        labelState.setText(text);
        stateON(labelState);
    }

    private void stateON(Label labelState) {
        FadeTransition ft = new FadeTransition(Duration.millis(500), labelState);
        ft.setFromValue(OFF_OPACITY);
        ft.setToValue(ON_OPACITY);
        ft.play();
    }

    private void stateOFF(Label labelState) {
        FadeTransition ft = new FadeTransition(Duration.millis(500), labelState);
        ft.setFromValue(ON_OPACITY);
        ft.setToValue(OFF_OPACITY);
        ft.play();
    }
}
