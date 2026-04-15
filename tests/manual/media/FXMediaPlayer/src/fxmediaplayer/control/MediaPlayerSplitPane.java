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
import javafx.scene.Node;
import javafx.scene.control.SplitPane;
import javafx.scene.control.TabPane;
import javafx.scene.media.MediaPlayer;

public class MediaPlayerSplitPane implements FXMediaPlayerControlInterface {

    private FXMediaPlayerInterface FXMediaPlayer = null;
    private SplitPane splitPane = null;
    private MediaPlayerControlNode controlNode = null;
    private MediaPlayerEffectsNode effectsNode = null;
    private MediaPlayerEqualizerNode equalizerNode = null;
    private MediaPlayerSpectrumNode spectrumNode = null;

    public MediaPlayerSplitPane(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;
    }

    public Node getNode() {
        if (splitPane == null) {
            splitPane = new SplitPane();

            controlNode = new MediaPlayerControlNode(FXMediaPlayer);
            effectsNode = new MediaPlayerEffectsNode(FXMediaPlayer);
            equalizerNode = new MediaPlayerEqualizerNode(FXMediaPlayer);
            spectrumNode = new MediaPlayerSpectrumNode(FXMediaPlayer);

            splitPane.getItems().add(controlNode.getNode());
            splitPane.getItems().add(effectsNode.getNode());
            splitPane.getItems().add(equalizerNode.getNode());
            splitPane.getItems().add(spectrumNode.getNode());
        }

        return splitPane;
    }

    @Override
    public void onMediaPlayerChanged(MediaPlayer oldMediaPlayer) {
        controlNode.onMediaPlayerChanged(oldMediaPlayer);
        effectsNode.onMediaPlayerChanged(oldMediaPlayer);
        equalizerNode.onMediaPlayerChanged(oldMediaPlayer);
        spectrumNode.onMediaPlayerChanged(oldMediaPlayer);
    }
}
