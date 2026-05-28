using System;
using System.IO;
using System.Text;
using WpfApp1.Models;

namespace WpfApp1.Services
{
    /// <summary>
    /// Schreibt empfangene Messwerte fortlaufend in eine CSV-Datei.
    /// Start() oeffnet/erzeugt die Datei und schreibt den Header,
    /// Append() haengt eine Zeile an, Stop() schliesst die Datei.
    /// </summary>
    public sealed class CsvLogger : IDisposable
    {
        private StreamWriter _writer;
        private readonly object _lock = new object();

        public bool IsLogging { get; private set; }
        public string CurrentPath { get; private set; }
        public int LineCount { get; private set; }

        public void Start(string path)
        {
            lock (_lock)
            {
                Stop();
                _writer = new StreamWriter(path, false, Encoding.UTF8);
                _writer.WriteLine(Measurement.CsvHeader());
                _writer.Flush();
                CurrentPath = path;
                LineCount = 0;
                IsLogging = true;
            }
        }

        public void Append(Measurement m)
        {
            lock (_lock)
            {
                if (!IsLogging || _writer == null) return;
                _writer.WriteLine(m.ToCsvLine());
                _writer.Flush();   // sofort sichern – Schulprojekt, Datenverlust vermeiden
                LineCount++;
            }
        }

        public void Stop()
        {
            lock (_lock)
            {
                if (_writer != null)
                {
                    _writer.Flush();
                    _writer.Dispose();
                    _writer = null;
                }
                IsLogging = false;
            }
        }

        public void Dispose() { Stop(); }
    }
}
