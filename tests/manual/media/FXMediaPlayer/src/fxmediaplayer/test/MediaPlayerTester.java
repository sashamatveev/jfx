package fxmediaplayer.test;

import fxmediaplayer.FXMediaPlayerInterface;
import fxmediaplayer.test.tests.LoadTest;
import fxmediaplayer.test.tests.PlayTest;
import javafx.event.ActionEvent;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ToolBar;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

public class MediaPlayerTester {
    private FXMediaPlayerInterface FXMediaPlayer = null;
    private MediaPlayerController controller = null;

    private VBox vbox = null;
    private ToolBar toolBar = null;
    private Button buttonStart = null;
    private Button buttonNext = null;

    // Tests
    private Test currentTest = null;
    private Test loadTest = null;
    private Test playTest = null;

    public MediaPlayerTester(FXMediaPlayerInterface FXMediaPlayer) {
        this.FXMediaPlayer = FXMediaPlayer;
        controller = new MediaPlayerController(FXMediaPlayer);

        loadTests("http://localhost/media/HCTFull.mp4");
    }

    public void openTesterWindow() {
        toolBar = new ToolBar();

        buttonStart = new Button("Start");
        buttonStart.setOnAction((ActionEvent event) -> {
            onButtonStart();
        });
        toolBar.getItems().add(buttonStart);

        buttonNext = new Button("Next");
        buttonNext.setOnAction((ActionEvent event) -> {
            onButtonNext();
        });
        toolBar.getItems().add(buttonNext);

        vbox = new VBox(5);
        vbox.getChildren().add(toolBar);

        Scene scene = new Scene(vbox, 230, 100);

        // New window (Stage)
        Stage newWindow = new Stage();
        newWindow.setTitle("Media Player Tester");
        newWindow.setScene(scene);

        // Set position of second window, related to primary window.
        //newWindow.setX(primaryStage.getX() + 200);
        //newWindow.setY(primaryStage.getY() + 100);
        newWindow.show();
    }

    public MediaPlayerController getController() {
        return controller;
    }

    private void loadTests(String source) {
        loadTest = new LoadTest(controller, source);
        playTest = new PlayTest(controller);

        currentTest = loadTest;
    }

    private void onButtonStart() {
        currentTest.start();
    }

    private void onButtonNext() {
    }
}
