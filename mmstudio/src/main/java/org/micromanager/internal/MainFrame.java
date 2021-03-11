///////////////////////////////////////////////////////////////////////////////
//PROJECT:       Micro-Manager
//SUBSYSTEM:     mmstudio
//-----------------------------------------------------------------------------
//
// AUTHOR:       
//
// COPYRIGHT:    University of California, San Francisco, 2014
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

package org.micromanager.internal;

import com.bulenkov.iconloader.IconLoader;
import com.google.common.eventbus.Subscribe;
import java.awt.Color;
import java.awt.Font;
import java.awt.Insets;
import java.awt.KeyboardFocusManager;
import java.awt.Toolkit;
import java.awt.dnd.DropTarget;
import java.awt.event.ActionEvent;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import javax.swing.BorderFactory;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.JToggleButton;
import javax.swing.border.MatteBorder;
import mmcorej.CMMCore;
import net.miginfocom.swing.MigLayout;
import org.micromanager.alerts.internal.AlertClearedEvent;
import org.micromanager.alerts.internal.AlertUpdatedEvent;
import org.micromanager.alerts.internal.DefaultAlertManager;
import org.micromanager.alerts.internal.DefaultAlert;
import org.micromanager.alerts.internal.NoAlertsAvailableEvent;
import org.micromanager.events.ChannelExposureEvent;
import org.micromanager.events.ConfigGroupChangedEvent;
import org.micromanager.events.GUIRefreshEvent;
import org.micromanager.events.internal.MouseMovesStageStateChangeEvent;
import org.micromanager.events.internal.ShutterDevicesEvent;
import org.micromanager.internal.dialogs.OptionsDlg;
import org.micromanager.internal.utils.*;
import org.micromanager.quickaccess.internal.QuickAccessFactory;
import org.micromanager.quickaccess.internal.controls.ShutterControl;

/**
 * GUI code for the primary window of the program. And nothing else.
 */
public final class MainFrame extends JFrame {

   private static final String MICRO_MANAGER_TITLE = "Micro-Manager";
   private static final String MAIN_EXPOSURE = "exposure";

   // Size constraint for normal buttons.
   private static final String BIGBUTTON_SIZE = "w 110!, h 22!";
   // Size constraint for small buttons.
   private static final String SMALLBUTTON_SIZE = "w 30!, h 20!";

   // GUI components
   private JComboBox<String> shutterComboBox_;
   private JTextField textFieldExp_;

   private JLabel labelImageDimensions_;
   private JButton liveButton_;
   private JCheckBox autoShutterCheckBox_;
   private JLabel shutterIcon_;
   private JButton toggleShutterButton_;
   private JButton snapButton_;
   private JToggleButton handMovesButton_;
   private JButton autofocusNowButton_;
   private JButton autofocusConfigureButton_;
   private JButton saveConfigButton_;
   private JLabel alertLabel_;
   private JButton alertButton_;
   private DefaultAlert displayedAlert_;

   private ConfigGroupPad configPad_;

   private final Font defaultFont_ = new Font("Arial", Font.PLAIN, 10);

   private final CMMCore core_;
   private final MMStudio mmStudio_;

   private ConfigPadButtonPanel configPadButtonPanel_;

   @SuppressWarnings("LeakingThisInConstructor")
   public MainFrame(MMStudio mmStudio, CMMCore core) {
      super("main micro manager frame");
      org.micromanager.internal.diagnostics.ThreadExceptionLogger.setUp();

      mmStudio_ = mmStudio;
      core_ = core;

      super.setTitle(String.format("%s %s", MICRO_MANAGER_TITLE,
               MMVersion.VERSION_STRING));

      JPanel contents = new JPanel();
      // Minimize insets.
      contents.setLayout(new MigLayout("insets 1, gap 0, fill"));
      super.setContentPane(contents);

      contents.add(createComponents(), "grow");

      super.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
      setupWindowHandlers();

      // Add our own keyboard manager that handles Micro-Manager shortcuts
      MMKeyDispatcher mmKD = new MMKeyDispatcher();
      KeyboardFocusManager.getCurrentKeyboardFocusManager().addKeyEventDispatcher(mmKD);
      DropTarget dropTarget = new DropTarget(this, new DragDropUtil(mmStudio_));

      setExitStrategy(OptionsDlg.getShouldCloseOnExit(mmStudio_));

      super.setJMenuBar(mmStudio.uiManager().menubar());

      // Set minimum size so we can't resize smaller and hide some of our
      // contents. Our insets are only available after the first call to
      // pack().
      super.pack();
      super.setIconImage(Toolkit.getDefaultToolkit().getImage(
              getClass().getResource("/org/micromanager/icons/microscope.gif")));
      super.setMinimumSize(super.getSize());
      super.setBounds(100, 100, 644, 220);
      WindowPositioning.setUpBoundsMemory(this, this.getClass(), null);
      mmStudio_.events().registerForEvents(this);
   }

   private void setupWindowHandlers() {
      addWindowListener(new WindowAdapter() {
         // HACK: on OSX, some kind of system bug can disable the entire
         // menu bar at times (it has something to do with modal dialogs and
         // possibly with errors resulting from the code that handles their
         // output). Calling setEnabled() on the MenuBar does *not* fix the
         // enabled-ness of the menus. However, through experimentation, I've
         // figured out that setting the menubar to null and then back again
         // does fix the issue for all menus *except* the Help menu. Note that
         // if we named our Help menu e.g. "Help2" then it would behave
         // properly, so this is clearly something special to do with OSX.
         @Override
         public void windowActivated(WindowEvent event) {
            setMenuBar(null);
            setJMenuBar(getJMenuBar());
         }
         // Shut down when this window is closed.
         @Override
         public void windowClosing(WindowEvent event) {
            mmStudio_.closeSequence(false);
         }
      });
   }

   public void initializeConfigPad() {
      configPad_.initialize();
      configPadButtonPanel_.initialize();
   }

   private JButton createButton(String text, String iconPath,
         String help, final Runnable action) {
      Icon icon = null;
      if (iconPath != null) {
         icon = IconLoader.getIcon("/org/micromanager/icons/" + iconPath);
      }
      JButton button = new JButton(text, icon);
      button.setMargin(new Insets(0, 0, 0, 0));
      button.setToolTipText(help);
      button.setFont(defaultFont_);
      button.addActionListener((ActionEvent e) -> {
         action.run();
      });
      return button;
   }

   private static JLabel createLabel(String text, boolean big) {
      final JLabel label = new JLabel();
      label.setFont(new Font("Arial",
              big ? Font.BOLD : Font.PLAIN,
              big ? 11 : 10));
      label.setText(text);
      return label;
   }

   private JPanel createImagingSettingsWidgets() {
      JPanel subPanel = new JPanel(
            new MigLayout("flowx, fillx, insets 1, gap 0"));
      // HACK: This minor extra vertical gap aligns this text with the
      // Configuration Settings header over the config pad.
      subPanel.add(createLabel("Imaging settings", true), "gaptop 2, wrap");

      // Exposure time.
      subPanel.add(createLabel("Exposure [ms]", false), "split 2, gapy 0 15");

      textFieldExp_ = new JTextField(8);
      textFieldExp_.addFocusListener(new FocusAdapter() {
         @Override
         public void focusLost(FocusEvent fe) {
            mmStudio_.app().setExposure(getDisplayedExposureTime());
         }
      });
      textFieldExp_.setFont(defaultFont_);
      textFieldExp_.addActionListener((ActionEvent e) -> {
         mmStudio_.app().setExposure(getDisplayedExposureTime());
      });
      subPanel.add(textFieldExp_, "gapleft push, wrap");

      // Shutter device.
      JPanel shutterPanel = new JPanel(
            new MigLayout("fillx, flowx, insets 0, gap 0"));
      shutterPanel.setBorder(BorderFactory.createLoweredBevelBorder());
      shutterPanel.add(createLabel("Shutter", false), "split 2");

      shutterComboBox_ = new JComboBox<>();
      shutterComboBox_.setName("Shutter");
      shutterComboBox_.setFont(defaultFont_);
      shutterComboBox_.addActionListener((ActionEvent arg0) -> {
         try {
            if (shutterComboBox_.getSelectedItem() != null) {
               core_.setShutterDevice((String) shutterComboBox_.getSelectedItem());
            }
         } catch (Exception e) {
            ReportingUtils.showError(e);
         }
      });
      shutterPanel.add(shutterComboBox_, "gapleft push, width 60::, wrap");

      // Auto/manual shutter control.
      autoShutterCheckBox_ = ShutterControl.makeAutoShutterCheckBox(mmStudio_);
      shutterPanel.add(autoShutterCheckBox_, "w 72!, h 20!, split 3");

      shutterIcon_ = ShutterControl.makeShutterIcon(mmStudio_);
      shutterPanel.add(shutterIcon_, "growx, alignx center");

      toggleShutterButton_ = ShutterControl.makeShutterButton(mmStudio_);
      shutterPanel.add(toggleShutterButton_, "growx, h 20!, wrap");
      subPanel.add(shutterPanel);
      return subPanel;
   }

   private JPanel createConfigurationControls() {
      JPanel subPanel = new JPanel(
            new MigLayout("fill, flowy, insets 1, gap 0"));
      subPanel.add(createLabel("Configuration settings", true),
            "flowx, growx, pushy 0, split 2");

      saveConfigButton_ = createButton("Save", null,
         "Save current presets to the configuration file", () -> {
            mmStudio_.uiManager().promptToSaveConfigPresets();
      });
      subPanel.add(saveConfigButton_,
            "pushy 0, gapleft push, alignx right, w 88!, h 20!");

      configPad_ = new ConfigGroupPad(mmStudio_);
      configPadButtonPanel_ = new ConfigPadButtonPanel(mmStudio_, configPad_);
      configPad_.setFont(defaultFont_);

      // Allowing the config pad to grow horizontally and vertically requires
      // us to override its preferred size.
      subPanel.add(configPad_,
            "grow, pushy 100, alignx center, w min::9999, h min::9999, span");
      subPanel.add(configPadButtonPanel_,
            "growx, pushy 0, alignx left, w 320!, h 20!, span");
      return subPanel;
   }

   /** 
    * Generate the "Snap", "Live", "Snap to album", "MDA", and "Refresh"
    * buttons.
    */
   private JPanel createCommonActionButtons() {
      JPanel subPanel = new JPanel(
            new MigLayout("flowy, insets 1, gap 1"));
      snapButton_ = (JButton) QuickAccessFactory.makeGUI(mmStudio_.plugins().getQuickAccessPlugins().get(
               "org.micromanager.quickaccess.internal.controls.SnapButton"));
      snapButton_.setFont(defaultFont_);
      subPanel.add(snapButton_, BIGBUTTON_SIZE);

      liveButton_ = (JButton) QuickAccessFactory.makeGUI(mmStudio_.plugins().getQuickAccessPlugins().get(
               "org.micromanager.quickaccess.internal.controls.LiveButton"));
      liveButton_.setFont(defaultFont_);
      subPanel.add(liveButton_, BIGBUTTON_SIZE + ",  gapy 0 10");  //Put a gap below 


      // Autofocus
      // Icon based on the public-domain icon at
      // http://www.clker.com/clipart-267005.html
      autofocusNowButton_ = createButton("Auto Focus", "binoculars.png",
         "Autofocus now", () -> {
            mmStudio_.autofocusNow();
      });
      subPanel.add(autofocusNowButton_, BIGBUTTON_SIZE);

      // Icon based on the public-domain icon at
      // http://publicdomainvectors.org/en/free-clipart/Adjustable-wrench-icon-vector-image/23097.html
      autofocusConfigureButton_ = createButton("AF Settings",
            "wrench.png", "Set autofocus options", () -> {
               mmStudio_.app().showAutofocusDialog();
      });
      subPanel.add(autofocusConfigureButton_, BIGBUTTON_SIZE);

      return subPanel;
   }

   private JPanel createAlertPanel() {
      JPanel result = new JPanel(new MigLayout("flowx, insets 2, gap 0"));
      // This icon adapted from the public domain icon at
      // https://commons.wikimedia.org/wiki/File:Echo_bell.svg
      alertButton_ = createButton(null, "bell.png",
            "You have messages requesting your attention. Click to show the Messages window.",
            () -> {
               ((DefaultAlertManager) mmStudio_.alerts()).alertsWindow().showWithoutFocus();
      });
      alertButton_.setVisible(false);
      result.add(alertButton_, "width 30!, height 20!, hidemode 2");
      alertLabel_ = new JLabel("");
      alertLabel_.setFont(defaultFont_);
      result.add(alertLabel_, "width 260!, hidemode 2");
      return result;
   }

   @Subscribe
   public void onAlertUpdated(AlertUpdatedEvent event) {
      displayedAlert_ = event.getAlert();
      String title = displayedAlert_.getTitle();
      String text = displayedAlert_.getText();
      String newText = "";
      if (title != null) {
         newText += title + ((text != null) ? ": " : "");
      }
      if (text != null) {
         newText += text;
      }
      alertLabel_.setText(newText);
      alertButton_.setVisible(true);
      alertLabel_.setVisible(true);
      alertLabel_.invalidate();
   }

   @Subscribe
   public void onAlertCleared(AlertClearedEvent event) {
      if (event.getAlert() == displayedAlert_) {
         // Hide the alert text, but leave the icon visible since other alerts
         // may still be around.
         alertLabel_.setVisible(false);
      }
   }

   @Subscribe
   public void onNoAlertsAvailable(NoAlertsAvailableEvent event) {
      alertButton_.setVisible(false);
      alertLabel_.setVisible(false);
   }

   private JPanel createComponents() {
      JPanel overPanel = new JPanel(new MigLayout("fill, flowx, insets 1, gap 0"));
      JPanel subPanel = new JPanel(new MigLayout("flowx, insets 1, gap 0"));
      subPanel.add(createCommonActionButtons(), "growy, aligny top");
      subPanel.add(createImagingSettingsWidgets(), "gapleft 10, growx, wrap");
      subPanel.add(createAlertPanel(), "span, wrap");
      overPanel.add(subPanel, "gapbottom push, grow 0, pushx 0");
      overPanel.add(createConfigurationControls(), "grow, wrap, pushx 100");
      // Must not be a completely empty label or else our size calculations
      // fail when setting the minimum size of the frame.
      labelImageDimensions_ = createLabel(" ", false);
      labelImageDimensions_.setBorder(new MatteBorder(1, 0, 0, 0,
               new Color(200, 200, 200)));
      overPanel.add(labelImageDimensions_, "growx, pushy 0, span, gap 2 0 2 0");
      return overPanel;
   }



   public final void setExitStrategy(boolean closeOnExit) {
      if (closeOnExit) {
         setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      }
      else {
         setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
      }
   }

   protected void setConfigSaveButtonStatus(boolean changed) {
      saveConfigButton_.setEnabled(changed);
   }

   /**
    * Updates Status line in main window.
    * @param text text to be shown 
    */
   public void updateInfoDisplay(String text) {
      labelImageDimensions_.setText(text);
   }

   public void toggleShutter() {
      try {
         if (!toggleShutterButton_.isEnabled()) {
            return;
         }
         toggleShutterButton_.requestFocusInWindow();
         mmStudio_.shutter().setShutter(!mmStudio_.shutter().getShutter());
      } catch (Exception e1) {
         ReportingUtils.showError(e1);
      }
   }

   @Subscribe
   public void onShutterDevices(ShutterDevicesEvent event) {
      refreshShutterGUI();
   }

   private void refreshShutterGUI() {
      List<String> devices = mmStudio_.shutter().getShutterDevices();
      if (devices == null) {
         // No shutter devices available yet.
         return;
      }
      String[] items = new ArrayList<>(devices).toArray(new String[] {});
      GUIUtils.replaceComboContents(shutterComboBox_, items);
      String activeShutter = null;
      try {
         activeShutter = mmStudio_.shutter().getCurrentShutter();
      }
      catch (Exception e) {
         ReportingUtils.logError(e, "Error getting shutter device");
      }
      if (activeShutter != null) {
         shutterComboBox_.setSelectedItem(activeShutter);
      }
      else {
         shutterComboBox_.setSelectedItem("");
      }
   }

   public void updateAutofocusButtons(boolean isEnabled) {
      autofocusConfigureButton_.setEnabled(isEnabled);
      autofocusNowButton_.setEnabled(isEnabled);
   }

   @Subscribe
   public void onConfigGroupChanged(ConfigGroupChangedEvent event) {
      configPad_.refreshGroup(event.getGroupName(), event.getNewConfig());
   }

   @Subscribe
   public void onGUIRefresh(GUIRefreshEvent event) {
      refreshShutterGUI();
   }

   @Subscribe
   public void onMouseMovesStage(MouseMovesStageStateChangeEvent event) {
      setHandMovesButton(event.isEnabled());
   }

   private void setHandMovesButton(boolean state) {
      String icon = state ? "move_hand_on.png" : "move_hand.png";
      handMovesButton_.setIcon(IconLoader.getIcon(
              "/org/micromanager/icons/" + icon));
      handMovesButton_.setSelected(state);
   }

   @Subscribe
   public void onChannelExposure(ChannelExposureEvent event) {
      if (event.isMainExposureTime()) {
         setDisplayedExposureTime(event.getNewExposureTime());
      }
   }

   private List<String> sortBinningItems(final List<String> items) {
      ArrayList<Integer> binSizes = new ArrayList<>();

      // Check if all items are valid integers
      for (String s : items) {
         Integer i;
         try {
            i = Integer.valueOf(s);
         }
         catch (NumberFormatException e) {
            // Not a number; give up sorting and return original list
            return items;
         }
         binSizes.add(i);
      }

      Collections.sort(binSizes);
      ArrayList<String> ret = new ArrayList<>();
      for (Integer i : binSizes) {
         ret.add(i.toString());
      }
      return ret;
   }

   public void configureBinningComboForCamera(String cameraLabel) {
      /*ActionListener[] listeners;

      try {
         StrVector binSizes = core_.getAllowedPropertyValues(
                 cameraLabel, MMCoreJ.getG_Keyword_Binning());
         List<String> items = sortBinningItems(Arrays.asList(binSizes.toArray()));

         listeners = comboBinning_.getActionListeners();
         for (ActionListener listener : listeners) {
            comboBinning_.removeActionListener(listener);
         }

         if (comboBinning_.getItemCount() > 0) {
            comboBinning_.removeAllItems();
         }

         for (String item : items) {
            comboBinning_.addItem(item);
         }

         comboBinning_.setMaximumRowCount(items.size());
         if (items.isEmpty()) {
             comboBinning_.setEditable(true);
         } else {
             comboBinning_.setEditable(false);
         }

         for (ActionListener listener : listeners) {
            comboBinning_.addActionListener(listener);
         }
      } catch (Exception e) {
         // getAllowedPropertyValues probably failed.
         ReportingUtils.showError(e);
      }*/
   }

   public void setBinSize(String binSize) {
      //GUIUtils.setComboSelection(comboBinning_, binSize);
   }

   public ConfigGroupPad getConfigPad() {
      return configPad_;
   }

   /**
    * Save our settings to the user profile.
    */
   public void savePrefs() {
      mmStudio_.profile().getSettings(MainFrame.class).putString(
            MAIN_EXPOSURE, textFieldExp_.getText());
   }

   public void setDisplayedExposureTime(double exposure) {
      textFieldExp_.setText(NumberUtils.doubleToDisplayString(exposure));
   }

   public double getDisplayedExposureTime() {
      try {
         return NumberUtils.displayStringToDouble(textFieldExp_.getText());
      } catch (ParseException e) {
         ReportingUtils.logError(e, "Couldn't convert displayed exposure time to double");
      }
      return -1;
   }
}
