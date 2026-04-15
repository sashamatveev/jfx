package fxmediaplayer.test.tests;

import fxmediaplayer.test.MediaPlayerController;
import fxmediaplayer.test.Test;

public class LoadTest extends Test {
    private String source = null;

    public LoadTest(MediaPlayerController controller, String source) {
        super(controller);

        this.source = source;
    }

	@Override
	public void start() {
	}

}
