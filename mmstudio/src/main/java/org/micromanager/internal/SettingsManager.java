/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.micromanager.internal;

import org.micromanager.Studio;

/**
 *
 * @author nicke
 */
public class SettingsManager {
   private final Studio studio_;
   private static final String SHOULD_DELETE_OLD_CORE_LOGS = "whether or not to delete old MMCore log files";
   private static final String SHOULD_RUN_ZMQ_SERVER = "run ZQM server";
   private static final String CORE_LOG_LIFETIME_DAYS = "how many days to keep MMCore log files, before they get deleted";
   private static final String CIRCULAR_BUFFER_SIZE = "size, in megabytes of the circular buffer used to temporarily store images before they are written to disk";

   public SettingsManager(Studio studio) {
      studio_ = studio;
   }

   public boolean getShouldDeleteOldCoreLogs() {
      return studio_.profile().getSettings(MMStudio.class).getBoolean(
            SHOULD_DELETE_OLD_CORE_LOGS, false);
   }

   public void setShouldDeleteOldCoreLogs(boolean shouldDelete) {
      studio_.profile().getSettings(MMStudio.class).putBoolean(
            SHOULD_DELETE_OLD_CORE_LOGS, shouldDelete);
   }

   public boolean getShouldRunZMQServer() {
      return studio_.profile().getSettings(MMStudio.class).getBoolean(
              SHOULD_RUN_ZMQ_SERVER, false);
   }

   public void setShouldRunZMQServer(boolean shouldRun) {
      studio_.profile().getSettings(MMStudio.class).putBoolean(
              SHOULD_RUN_ZMQ_SERVER, shouldRun);
   }

   public int getCoreLogLifetimeDays() {
      return studio_.profile().getSettings(MMStudio.class).getInteger(
            CORE_LOG_LIFETIME_DAYS, 7);
   }

   public void setCoreLogLifetimeDays(int days) {
      studio_.profile().getSettings(MMStudio.class).putInteger(
            CORE_LOG_LIFETIME_DAYS, days);
   }

   public int getCircularBufferSize() {
      // Default to more MB for 64-bit systems.
      int defaultVal = System.getProperty("sun.arch.data.model", "32").equals("64") ? 250 : 25;
      return studio_.profile().getSettings(MMStudio.class).getInteger(
            CIRCULAR_BUFFER_SIZE, defaultVal);
   }

   public void setCircularBufferSize(int newSize) {
      studio_.profile().getSettings(MMStudio.class).putInteger(
            CIRCULAR_BUFFER_SIZE, newSize);
   }
}