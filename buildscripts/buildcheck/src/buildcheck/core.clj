(ns buildcheck.core
  (:import (java.io File))
  (:use [local-file :only (file*)]
        [clj-mail.core])
  (:gen-class))

(def micromanager (file* "../.."))

(def MS-PER-HOUR (* 60 60 1000))

(defn result-file [bits mode]
  (File. micromanager (str "/result" bits (name mode) ".txt")))

(defn visual-studio-errors [result-text]
  (map first
       (re-seq #"\n[^\n]+\b([1-9]|[0-9][1-9]|[0-9][0-9][1-9])\b\serror\(s\)[^\n]+\n" result-text)))

(defn visual-studio-error-info [result-text errors]
  (for [error errors]
    (let [prefix (ffirst (re-seq #"\n([1-9]|[0-9][1-9]|[0-9][0-9][1-9])>" error))
          pattern (re-pattern (str prefix "[^\\n]+"))]
      (println prefix)
      (println "pattern:" pattern)
      (when prefix
          (re-seq (re-pattern pattern) result-text)))))

(defn visual-studio-error-text [result-text]
  (->> (visual-studio-error-info result-text (visual-studio-errors result-text))
       (interpose "\n")
       flatten
       (apply str)))

(defn javac-errors [result-text]
  (map first
    (re-seq #"\[javac\]\s\b([1-9]|[0-9][1-9]|[0-9][0-9][1-9])\b\serrors?" result-text)))

(defn old-files [files time-limit-hours]
  (let [now (System/currentTimeMillis)
        before (- now (* time-limit-hours MS-PER-HOUR))]
    (filter #(< (.lastModified %) before) files)))
    
(defn old-dlls [dir time-limit-hours]
  (old-files
    (filter
      (fn [file]
        (let [file-name (.getName file)]
          (and (.endsWith file-name ".dll")
               (.startsWith file-name "mmgr_dal"))))
      (.listFiles dir))
    time-limit-hours))

(defn old-jars [dir time-limit-hours]
  (old-files
    (filter
      #(.. % getName (endsWith ".jar"))
      (file-seq dir))
    time-limit-hours))

(defn report-build-errors [bits mode]
  (let [f (result-file bits mode)
        result-txt (slurp f)
        vs-error-text (visual-studio-error-text result-txt)
        outdated-dlls (old-dlls (File. micromanager
                                       (condp = bits
                                         32 "bin_Win32"
                                         64 "bin_x64")) 24)
        javac-errs (javac-errors result-txt)
        outdated-jars (old-jars (File. micromanager "Install_AllPlatforms") 24)]
    (when-not (and (empty? vs-error-text) (empty? outdated-dlls)
                   (empty? javac-errs) (empty? outdated-jars))
      (println (str "\n\nMICROMANAGER " bits "-bit "
                    ({:inc "INCREMENTAL" :full "FULL"} mode)
                    " BUILD ERROR REPORT"))
      (println (str "For the full build output, see " (.getAbsolutePath f)))
      (println "\nVisual Studio reported errors:")
      (if-not (empty? vs-error-text) (println vs-error-text) (println "None."))
      (println "\nOutdated device adapter DLLs:")
      (if-not (empty? outdated-dlls) (dorun (map #(println (.getName %)) outdated-dlls)) (println "None."))
      (println "\nErrors reported by java compiler:")
      (if-not (empty? javac-errs) (dorun (map println javac-errs)) (println "None."))
      (println "\nOutdated jar files:")
      (if-not (empty? outdated-jars) (dorun (map #(println (.getName %)) outdated-jars)) (println "None."))
      )))

(defn make-full-report [mode send?]
  (let [report
        (with-out-str
          (report-build-errors 32 mode)
          (report-build-errors 64 mode))]
    (if-not (empty? report)
      (do 
        (when send?
          (with-session
            "mmbuilderrors@gmail.com" (slurp "C:\\pass.txt") "smtp.gmail.com" 465 "smtp" true
            (send-email (text-email ["info@micro-manager.org"] "mm build errors" report))))
        (println report))
      (println "Nothing to report."))))

(defn -main [mode]
  (make-full-report (get {"inc" :inc "full" :full} mode) true))

    
 