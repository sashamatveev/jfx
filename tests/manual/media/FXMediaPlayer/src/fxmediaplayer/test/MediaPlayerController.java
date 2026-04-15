package fxmediaplayer.test;

import fxmediaplayer.FXMediaPlayerInterface;
import javafx.collections.ListChangeListener;
import javafx.collections.ObservableList;
import javafx.scene.control.Button;

public class MediaPlayerController {

    private FXMediaPlayerInterface FXMediaPlayer = null;

    // Player controls play, pause, stop, etc...
    private Button buttonPlay = null;
    private Button buttonPause = null;
    private Button buttonStop = null;

    // Events
    private ListChangeListener<String> mediaEventListener = null;
    private ObservableList<String> mediaEvents = null;
    private Button buttonClearEvents = null;

    public MediaPlayerController(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;

        mediaEventListener = (ListChangeListener.Change<? extends String> change) -> {
            onMediaEvent(change);
        };
    }

    public void setPlayButton(Button buttonPlay) {
        this.buttonPlay = buttonPlay;
    }

    public void setPauseButton(Button buttonPause) {
        this.buttonPause = buttonPause;
    }

    public void setStopButton(Button buttonStop) {
        this.buttonStop = buttonStop;
    }

    public void setMediaEvents(ObservableList<String> mediaEvents) {
        if (this.mediaEvents != null) {
            this.mediaEvents.removeListener(mediaEventListener);
        }

        this.mediaEvents = mediaEvents;
        this.mediaEvents.addListener(mediaEventListener);
    }

    public void setClearEvents(Button buttonClearEvents) {
        this.buttonClearEvents = buttonClearEvents;
    }

    private void onMediaEvent(ListChangeListener.Change<? extends String> change) {
        while (change.next()) {
            if (change.wasAdded()) {
                change.getAddedSubList().forEach(event -> {
                    System.out.println("MediaPlayerController::onMediaEvent() " + event);
                });
            }
        }
    }

    public void play() {
        buttonPlay.fire();
    }

    public void pause() {
        buttonPause.fire();
    }

    public void stop() {
        buttonStop.fire();
    }
}
