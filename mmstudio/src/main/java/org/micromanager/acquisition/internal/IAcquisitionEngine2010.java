///////////////////////////////////////////////////////////////////////////////
// PROJECT:       Micro-Manager
// SUBSYSTEM:     mmstudio
// -----------------------------------------------------------------------------
//
// AUTHOR:       Arthur Edelstein
//
// COPYRIGHT:    University of California, San Francisco, 2010
//
// LICENSE:      This file is distributed under the BSD license.
//               License text is included with the source distribution.
//
//               This file is distributed in the hope that it will be useful,
//               but WITHOUT ANY WARRANTY; without even the implied warranty
//               of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//               IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//               CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//               INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.

package org.micromanager.acquisition.internal;

import java.util.concurrent.BlockingQueue;
import mmcorej.TaggedImage;
import mmcorej.org.json.JSONObject;
import org.micromanager.AutofocusPlugin;
import org.micromanager.PositionList;
import org.micromanager.acquisition.SequenceSettings;

/**
 * This interface is implemented by AcquisitionEngine2010 (generated by the clojure code in
 * micromanager/acqEngine).
 */
public interface IAcquisitionEngine2010 {

  /*
   * When run is called, the implementing class should run a multi-dimensional
   * acquisition according to the specifications in the argument sequenceSettings.
   * Everything is returned to initial state after the acquisition is run (cleanup = true).
   */
  public BlockingQueue<TaggedImage> run(SequenceSettings sequenceSettings);

  /*
   * When run is called, the implementing class should run a multi-dimensional
   * acquisition according to the specifications in the argument sequenceSettings.
   * The specified PositionList and AutofocusPlugin device are used.
   * Everything is returned to initial state after the acquisition is run.
   */
  public BlockingQueue<TaggedImage> run(
      SequenceSettings sequenceSettings,
      boolean cleanup,
      PositionList positionList,
      AutofocusPlugin device);

  /*
   * When run is called, the implementing class should run a multi-dimensional
   * acquisition according to the specifications in the argument sequenceSettings.
   * If cleanup is true, everything is returned to initial state after
   * the acquisition is run.
   */
  public BlockingQueue<TaggedImage> run(SequenceSettings sequenceSettings, boolean cleanup);

  /*
   * Returns the summaryMetadata for the most recently started acquisition
   * sequence.
   */
  public JSONObject getSummaryMetadata();

  /*
   * Pause the ongoing acquisition as soon as possible.
   */
  public void pause();

  /*
   * Resume a paused acquisition.
   */
  public void resume();

  /*
   * Permanently halt an acquisition as soon as possible.
   */
  public void stop();

  /*
   * Returns true if the acquisition is running, even if it is paused.
   */
  public boolean isRunning();

  /*
   * Returns true if the acquisition has paused.
   */
  public boolean isPaused();

  /*
   * Returns true if the acquisition has completed (i.e., there are
   * no pending or ongoing hardware tasks).
   */
  public boolean isFinished();

  /*
   * Returns true if the client has requested stop.
   */
  public boolean stopHasBeenRequested();

  /*
   * Returns the time in milliseconds (on computer clock time) when the
   * current sleep is expected to finish.
   */
  public long nextWakeTime();

  /*
   * Attach a Runnable object to the acquisition, such that an extra
   * hardware event can be carried out at a specified point in a
   * multi-dimensional sequence. If -1 (negative one) is used for
   * any of the dimensions (frame, position, slice, channel), then
   * the runnable is invoked for every possible value of that dimension.
   * For example, passing arguments (-1, 0, -1, 2, theRunnable) would result
   * in theRunnable being executed at every frame, position 0, every channel,
   * and slice 2.
   */
  public void attachRunnable(int frame, int position, int channel, int slice, Runnable runnable);

  /*
   * Removes all runnables that have been attached to this
   * acquisition engine.
   */
  public void clearRunnables();
}
