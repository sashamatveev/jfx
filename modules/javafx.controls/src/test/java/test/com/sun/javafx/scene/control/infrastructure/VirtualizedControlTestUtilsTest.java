/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates. All rights reserved.
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

package test.com.sun.javafx.scene.control.infrastructure;

import static test.com.sun.javafx.scene.control.infrastructure.VirtualizedControlTestUtils.*;

import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.scene.Scene;
import javafx.scene.control.Control;
import javafx.scene.control.ListView;
import javafx.scene.control.ScrollBar;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertNotSame;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import static org.junit.jupiter.api.Assertions.assertThrows;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

/**
 * Rudimentary test of VirtualizedControlTestUtils.
 */
public class VirtualizedControlTestUtilsTest {

    private Scene scene;
    private Stage stage;
    private Pane root;

    int rows;

    /**
     * Test that firing on the vertical bar scrolls the control.
     */
    @Test
    public void testFireMouseOnVerticalTrack() {
        ListView<?> list = createAndShowListView();
        ScrollBar scrollBar = getVerticalScrollBar(list);
        assertEquals(0, scrollBar.getValue(), 0.1, "sanity: initial value of scrollBar");
        fireMouseOnVerticalTrack(list);
        assertTrue(scrollBar.getValue() > 0, "mouse on track must have scrolled");
    }

    /**
     * Test that firing on the horizontal bar scrolls the control.
     */
    @Test
    public void testFireMouseOnHorizontalTrack() {
        ListView<?> list = createAndShowListView();
        ScrollBar scrollBar = getHorizontalScrollBar(list);
        assertEquals(0, scrollBar.getValue(), 0.1, "sanity: initial value of scrollBar");
        fireMouseOnHorizontalTrack(list);
        assertTrue(scrollBar.getValue() > 0, "mouse on track must have scrolled");
    }

    @Test
    public void testGetVerticalScrollBarThrowsWithoutSkin() {
        assertThrows(IllegalStateException.class, () -> {
            ListView<?> list = new ListView<>();
            getVerticalScrollBar(list);
        });
    }

    @Test
    public void testGetHorizontalScrollBarThrowsWithoutSkin() {
        assertThrows(IllegalStateException.class, () -> {
            ListView<?> list = new ListView<>();
            getHorizontalScrollBar(list);
        });
    }

    /**
     * Test the test setup.
     */
    @Test
    public void testListViewEditing() {
        ListView<?> control = createAndShowListView();
        assertEquals(rows, control.getItems().size());
        assertEquals(100, scene.getWidth(), 1);
        assertEquals(330, scene.getHeight(), 1);
        assertTrue(getHorizontalScrollBar(control).isVisible(), "sanity: vertical scrollbar visible for list ");
        assertTrue(getVerticalScrollBar(control).isVisible(), "sanity: vertical scrollbar visible for list ");
    }

  //----------------- setup

    /**
     * Creates and shows a ListView configured to be scrollable both vertically and horizontally.
     */
    private ListView<?> createAndShowListView() {
        ObservableList<String> baseData = createData(rows, true);
        ListView<String> control = new ListView<>(baseData);
        showControl(control, true, 100, 330);
        return control;
    }

    /**
     * Creates and returns a list of long/short (depending on wide parameter) Strings.
     */
    private ObservableList<String> createData(int size, boolean wide) {
        ObservableList<String> data = FXCollections.observableArrayList();
        String item = wide ? "something that really really guarantees a horizontal scrollbar is visible  " : "item";
        for (int i = 0; i < size; i++) {
            data.add(item + i);
        }
        return data;
    }

    /**
     * Ensures the control is shown in an active scenegraph. Requests
     * focus on the control if focused == true.
     *
     * @param control the control to show
     * @param focused if true, requests focus on the added control
     */
    protected void showControl(Control control, boolean focused) {
        showControl(control, focused, -1, -1);
    }

    /**
     * Ensures the control is shown in an active scenegraph. Requests
     * focus on the control if focused == true.
     * On first call, sizes the scene to width/height if width > 0
     *
     * @param control the control to show
     * @param focused if true, requests focus on the added control
     * @param width the width of the scene or -1 for auto-sizing
     * @param height the height of the scene or -1 for auto-sizing
     */
    protected void showControl(Control control, boolean focused, double width, double height) {
        if (root == null) {
            root = new VBox();
            if (width > 0) {
                scene = new Scene(root, width, height);
            } else {
                scene = new Scene(root);
            }
            stage = new Stage();
            stage.setScene(scene);
        }
        if (!root.getChildren().contains(control)) {
            root.getChildren().add(control);
        }
        stage.show();
        if (focused) {
            stage.requestFocus();
            control.requestFocus();
            assertTrue(control.isFocused());
            assertSame(control, scene.getFocusOwner());
        }
    }

    @BeforeEach
    public void setup() {
        Thread.currentThread().setUncaughtExceptionHandler((thread, throwable) -> {
            if (throwable instanceof RuntimeException) {
                throw (RuntimeException)throwable;
            } else {
                Thread.currentThread().getThreadGroup().uncaughtException(thread, throwable);
            }
        });
        rows = 60;
    }

    @AfterEach
    public void cleanup() {
        if (stage != null)
            stage.hide();
        Thread.currentThread().setUncaughtExceptionHandler(null);
    }
}
