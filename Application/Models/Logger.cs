using System;
using System.IO;
using System.Reflection;

namespace ToolKitV.Models
{
    public class LogWriter
    {
        private string m_exePath = string.Empty;
        public LogWriter(string logMessage)
        {
            InitLogFile();
            LogWrite(logMessage);
        }

        private void InitLogFile()
        {
            m_exePath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            try
            {
                using StreamWriter w = File.CreateText(m_exePath + "\\" + "log.txt");
                Log("Init log file", w);
            }
            catch (Exception)
            {
            }
        }
        public void LogWrite(string logMessage)
        {
            try
            {
                using StreamWriter w = File.AppendText(m_exePath + "\\" + "log.txt");
                Log(logMessage, w);
            }
            catch (Exception)
            {
            }
        }

        public static void Log(string logMessage, TextWriter txtWriter)
        {
            try
            {
                txtWriter.Write("{0} {1} | ", DateTime.Now.ToLongTimeString(),
                    DateTime.Now.ToLongDateString());
                txtWriter.WriteLine("{0}", logMessage);
            }
            catch (Exception)
            {
            }
        }
    }
}
