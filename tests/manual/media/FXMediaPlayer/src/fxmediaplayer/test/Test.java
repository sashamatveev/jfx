package fxmediaplayer.test;

import fxmediaplayer.FXMediaPlayerInterface;

public abstract class Test {
    private FXMediaPlayerInterface FXMediaPlayer = null;
    private MediaPlayerController controller = null;

    public Test(FXMediaPlayerInterface FXMediaPlayer, MediaPlayerController controller) {
        this.FXMediaPlayer = FXMediaPlayer;
        this.controller = controller;
    }

    public abstract void start();

    public FXMediaPlayerInterface getFXMediaPlayer() {
        return FXMediaPlayer;
    }

    public MediaPlayerController getController() {
        return controller;
    }
}
