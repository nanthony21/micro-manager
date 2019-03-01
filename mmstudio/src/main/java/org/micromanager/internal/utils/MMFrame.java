///////////////////////////////////////////////////////////////////////////////
//PROJECT:       Micro-Manager
//SUBSYSTEM:     mmstudio
//-----------------------------------------------------------------------------
//
// AUTHOR:       Nenad Amodaj, nenad@amodaj.com, October 1, 2006
//
// COPYRIGHT:    University of California, San Francisco, 2006
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
//
// CVS:          $Id$
//
package org.micromanager.internal.utils;

import java.awt.Frame;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import javax.swing.JFrame;
import org.micromanager.PropertyMap;
import org.micromanager.PropertyMaps;
import org.micromanager.internal.MMStudio;
import org.micromanager.internal.menus.MMMenuBar;
import org.micromanager.propertymap.MutablePropertyMapView;
import java.awt.event.WindowEvent;
import java.awt.event.WindowFocusListener;
import java.awt.event.WindowStateListener;
import java.util.ArrayList;
/**
 * Base class for Micro-Manager frame windows.
 * Saves and restores window size and position. 
 */
public class MMFrame extends JFrame implements WindowFocusListener, WindowStateListener {
   private static final long serialVersionUID = 1L;
   private final String profileKey_;
   private static ArrayList<MMFrame> applicationFrames = new ArrayList<MMFrame>();

   // MMFrame profile settings contain a key for each frame subtype (defined
   // by subclass), whose value is a property map containing these key(s):
   private static enum ProfileKey {
      WINDOW_BOUNDS,
   }

   public MMFrame() {
      super();
      profileKey_ = "";
      setupMenus();
      super.setIconImage(Toolkit.getDefaultToolkit().getImage(
        getClass().getResource("/org/micromanager/icons/microscope.gif")));
      addWindowFocusListener(this);
      addWindowStateListener(this);
   }

   public MMFrame(String profileKeyForSavingBounds) {
      super();
      profileKey_ = profileKeyForSavingBounds;
      setupMenus();
      super.setIconImage(Toolkit.getDefaultToolkit().getImage(
        getClass().getResource("/org/micromanager/icons/microscope.gif")));
      addWindowFocusListener(this);
      addWindowStateListener(this);

   }

   /**
    * @param usesMMMenus If true, then the Micro-Manager menubar will be shown
    * when the frame has focus. Is effectively true by default for the other
    * constructors.
    */
   public MMFrame(String profileKeyForSavingBounds, boolean usesMMMenus) {
      super();
      profileKey_ = profileKeyForSavingBounds;
      if (usesMMMenus) {
         setupMenus();
      }
      super.setIconImage(Toolkit.getDefaultToolkit().getImage(
        getClass().getResource("/org/micromanager/icons/microscope.gif")));
      addWindowFocusListener(this);
      addWindowStateListener(this);

   }

   /**
    * Create a copy of the Micro-Manager menus for use by this frame. Only
    * needed on OSX with its global (non-window-specific) menu bar.
    */
   private void setupMenus() {
      if (JavaUtils.isMac()) {
         setJMenuBar(MMMenuBar.createMenuBar(MMStudio.getInstance()));
      }
   }

   public void loadPosition(int x, int y, int width, int height) {
      MutablePropertyMapView settings =
            UserProfileStaticInterface.getInstance().getSettings(getClass());
      PropertyMap pmap = settings.getPropertyMap(profileKey_,
            PropertyMaps.emptyPropertyMap());
      Rectangle bounds = pmap.getRectangle(ProfileKey.WINDOW_BOUNDS.name(),
            new Rectangle(x, y, width, height));
      if (GUIUtils.getGraphicsConfigurationBestMatching(bounds) == null) {
         bounds.x = x;
         bounds.y = y;
      }
      setBounds(bounds);
      offsetIfNecessary();
   }

   public void loadPosition(int x, int y) {
      MutablePropertyMapView settings =
            UserProfileStaticInterface.getInstance().getSettings(getClass());
      PropertyMap pmap = settings.getPropertyMap(profileKey_,
            PropertyMaps.emptyPropertyMap());
      Rectangle bounds = pmap.getRectangle(ProfileKey.WINDOW_BOUNDS.name(),
            new Rectangle(x, y, getWidth(), getHeight()));
      if (GUIUtils.getGraphicsConfigurationBestMatching(bounds) == null) {
         bounds.x = x;
         bounds.y = y;
      }
      super.setLocation(bounds.x, bounds.y);
      offsetIfNecessary();
   }

   /**
    * Scan the program for other MMFrames that have the same type as this
    * MMFrame, and make certain we don't precisely overlap any of them.
    */
   private void offsetIfNecessary() {
      Point newLoc = getLocation();
      boolean foundOverlap = false;
      do {
         foundOverlap = false;
         for (Frame frame : Frame.getFrames()) {
            if (frame != this && frame.getClass() == getClass() &&
                  frame.isVisible() && frame.getLocation().equals(newLoc)) {
               foundOverlap = true;
               newLoc.x += 22;
               newLoc.y += 22;
            }
         }
         if (!foundOverlap) {
            break;
         }
      } while (foundOverlap);

      setLocation(newLoc);
   }

    /**
    * Load window position and size from profile if possible.
    * If not possible then sets them from arguments
    * Attaches a listener to the window that will save the position when the
    * window closing event is received
    * @param x - X position of this dialog if preference value invalid
    * @param y - y position of this dialog if preference value invalid
    * @param width - width of this dialog if preference value invalid
    * @param height - height of this dialog if preference value invalid
    */
   protected void loadAndRestorePosition(int x, int y, int width, int height) {
      loadPosition(x, y, width, height);
      addComponentListener(new ComponentAdapter() {
         @Override
         public void componentMoved(ComponentEvent e) {
            savePosition();
         }
      });
   }
   
    /**
    * Load window position from profile if possible.
    * If not possible then sets it from arguments
    * Attaches a listener to the window that will save the position when the
    * window closing event is received
    * @param x - X position of this dialog if preference value invalid
    * @param y - y position of this dialog if preference value invalid
    */
   protected void loadAndRestorePosition(int x, int y) {
      loadPosition(x, y);
      addComponentListener(new ComponentAdapter() {
         @Override
         public void componentMoved(ComponentEvent e) {
            savePosition();
         }
      });
   }
   

   public void savePosition() {
      Rectangle bounds = getBounds();
      MutablePropertyMapView settings =
            UserProfileStaticInterface.getInstance().getSettings(getClass());
      PropertyMap pmap = settings.getPropertyMap(profileKey_,
            PropertyMaps.emptyPropertyMap());
      pmap = pmap.copyBuilder().
            putRectangle(ProfileKey.WINDOW_BOUNDS.name(), bounds).build();
      settings.putPropertyMap(profileKey_, pmap);
   }

   @Override
   public void dispose() {
      applicationFrames.remove(this);
      savePosition();
      super.dispose();
   }
   
   @Override
   public void setVisible(boolean visible) {
       //The static applicationFrames list keeps track of all the MMFrames that are visible.
       if (visible) {
           applicationFrames.add(this);
       } else {
           applicationFrames.remove(this);
       }
       super.setVisible(visible);
   }
   
   @Override
   public void windowGainedFocus(WindowEvent e) {
       if (e.getOppositeWindow() == null) { //If the last selected windows was from a different application
           for (MMFrame frame : applicationFrames) {
               frame.toFront(); //Move all visible MMframes to the front
           }
       }
   }
   @Override
   public void windowLostFocus(WindowEvent e) {}
   
    @Override
    public void windowStateChanged(WindowEvent e) {
        if ((e.getNewState() == MMFrame.NORMAL) && (e.getOldState() == MMFrame.ICONIFIED)) {
            for (MMFrame frame : applicationFrames) {
                if (frame.getState() == MMFrame.ICONIFIED) {
                    frame.setState(MMFrame.NORMAL);
                }
            }
        }
    }
}
