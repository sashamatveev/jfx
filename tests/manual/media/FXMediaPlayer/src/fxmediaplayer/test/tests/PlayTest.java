package fxmediaplayer.test.tests;

import fxmediaplayer.test.MediaPlayerController;
import fxmediaplayer.test.Test;

public class PlayTest extends Test {

    public PlayTest(MediaPlayerController controller) {
        super(controller);
    }

    @Override
    public void start() {
        getController().play();
    }
}
