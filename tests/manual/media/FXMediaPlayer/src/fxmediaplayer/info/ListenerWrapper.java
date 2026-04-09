/*
 * Copyright (c) 2026, Oracle and/or its affiliates. All rights reserved.
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

package fxmediaplayer.info;

import fxmediaplayer.FXMediaPlayerUtils;
import javafx.beans.InvalidationListener;
import javafx.beans.Observable;
import javafx.collections.ObservableList;
import javafx.beans.property.ReadOnlyObjectProperty;
import javafx.util.Duration;

public class ListenerWrapper {
    private String label = null;
    private InvalidationListener listener = null;
    private int listViewIndex = -1;

    public ListenerWrapper(String label, InvalidationListener listener) {
        this.label = label;
        this.listener = listener;
    }

    public InvalidationListener getListener() {
        return listener;
    }

    public void reset() {
        listViewIndex = -1;
    }

    public void onDurationValue(ObservableList<String> list, Duration value) {
        addToList(list, getString(value));
    }

    public void onDoubleValue(ObservableList<String> list, double value) {
        addToList(list, getString(value));
    }

    private void addToList(ObservableList<String> list, String value) {
        if (listViewIndex == -1) {
            if (list.add(value)) {
                listViewIndex = list.size() - 1;
            }
        } else {
            list.set(listViewIndex, value);
        }
    }

    private String getString(Duration value) {
        return label + ": " + FXMediaPlayerUtils.secondsToString(value.toSeconds());
    }

    private String getString(double value) {
        return label + ": " + String.format("%.2f", value);
    }
}
