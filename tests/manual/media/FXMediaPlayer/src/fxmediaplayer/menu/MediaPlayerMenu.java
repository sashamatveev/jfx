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

package fxmediaplayer.menu;

import fxmediaplayer.FXMediaPlayerInterface;
import fxmediaplayer.test.MediaPlayerTester;
import javafx.event.ActionEvent;
import javafx.scene.control.Button;
import javafx.scene.control.ToggleButton;
import javafx.scene.control.ToolBar;
import javafx.scene.layout.VBox;

public class MediaPlayerMenu {

    private FXMediaPlayerInterface FXMediaPlayer = null;
    private VBox menu = null;
    private ToolBar toolBar = null;
    private ToolBar toolBarStates = null;
    private Button buttonOpen = null;
    private Button buttonDispose = null;
    private ToggleButton buttonAutoPlay = null;
    private Button buttonFullScreen = null;
    private ToggleButton buttonScrubbing = null;
    private Button buttonTest = null;
    private MediaPlayerMenuOpenDialog openDialog = null;
    private MediaPlayerTester mediaPlayerTester = null;

    public MediaPlayerMenu(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;
    }

    public VBox getMenu() {
        if (menu == null) {
            menu = new VBox();

            toolBar = new ToolBar();

            buttonOpen = new Button("Open");
            buttonOpen.setOnAction((ActionEvent event) -> {
                onButtonOpen();
            });
            toolBar.getItems().add(buttonOpen);

            buttonDispose = new Button("Dispose");
            buttonDispose.setOnAction((ActionEvent event) -> {
                onButtonDispose();
            });
            toolBar.getItems().add(buttonDispose);

            buttonAutoPlay = new ToggleButton("Auto Play");
            buttonAutoPlay.setOnAction((ActionEvent event) -> {
                onButtonAutoPlay();
            });
            toolBar.getItems().add(buttonAutoPlay);

            buttonFullScreen = new Button("Full Screen");
            buttonFullScreen.setOnAction((ActionEvent event) -> {
                onButtonFullScreen();
            });
            toolBar.getItems().add(buttonFullScreen);

            buttonScrubbing = new ToggleButton("Scrubbing");
            buttonScrubbing.setOnAction((ActionEvent event) -> {
                onButtonScrubbing();
            });
            toolBar.getItems().add(buttonScrubbing);

            buttonTest = new Button("Test");
            buttonTest.setOnAction((ActionEvent event) -> {
                onButtonTest();
            });
            toolBar.getItems().add(buttonTest);

            menu.getChildren().add(toolBar);

            toolBarStates = new ToolBar();

            menu.getChildren().add(toolBarStates);
        }

        return menu;
    }

    private void onButtonOpen() {
        if (openDialog == null) {
            openDialog = new MediaPlayerMenuOpenDialog(FXMediaPlayer);
        }

        openDialog.open();
    }

    private void onButtonDispose() {
        FXMediaPlayer.onSourceChanged(null);
    }

    private void onButtonAutoPlay() {
        FXMediaPlayer.setAutoPlay(buttonAutoPlay.isSelected());
    }

    private void onButtonFullScreen() {
        FXMediaPlayer.setFullScreen(true);
    }

    private void onButtonScrubbing() {
        FXMediaPlayer.setScrubbing(buttonScrubbing.isSelected());
    }

    private void onButtonTest() {
        if (mediaPlayerTester == null) {
            mediaPlayerTester = new MediaPlayerTester();
            FXMediaPlayer.setTester(mediaPlayerTester);
        }

        mediaPlayerTester.openTesterWindow();
    }
}
