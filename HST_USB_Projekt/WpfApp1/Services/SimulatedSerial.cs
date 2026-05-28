using System;
using System.Globalization;
using System.Windows.Threading;
using WpfApp1.Protocol;

namespace WpfApp1.Services
{
    /// <summary>
    /// Simulierter "COM-Port" – generiert realistische $DATA-Frames und
    /// beantwortet $IO-Befehle mit $ACK, exakt so wie es die echte Firmware tut.
    ///
    /// Genutzt vom <see cref="SerialService"/>, wenn der Portname == "SIM" ist.
    /// Laeuft komplett auf dem UI-Dispatcher (DispatcherTimer) – kein Threading-Stress.
    /// </summary>
    internal sealed class SimulatedSerial
    {
        private const double FrameHz = 5.0;     // 5 Frames/s (entspricht ~200 ms)
        private const int    AckDelayMs = 80;   // realistisches Antwortverhalten

        private readonly Dispatcher _ui;
        private readonly Action<string> _emitLine;   // Frame -> wie wenn er ueber UART kaeme
        private readonly Action<string> _status;     // Statusmeldung -> UI

        private readonly DispatcherTimer _dataTimer;
        private readonly Random _rng = new Random();

        // interner Zustand der "Sensoren"
        private double _shtT = 22.0;
        private double _shtH = 45.0;
        private double _ufT  = 28.0;
        private double _vbat = 3.30;
        private double _vsup = 5.00;
        private double _phase;

        public SimulatedSerial(Dispatcher ui,
                               Action<string> emitLine,
                               Action<string> status)
        {
            _ui = ui;
            _emitLine = emitLine;
            _status = status;

            _dataTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(1000.0 / FrameHz)
            };
            _dataTimer.Tick += (s, e) => GenerateDataFrame();
        }

        public bool IsRunning { get; private set; }

        public void Start()
        {
            if (IsRunning) return;
            IsRunning = true;
            _dataTimer.Start();
            if (_status != null) _status("Simulation gestartet (5 Hz, virtueller Port)");
        }

        public void Stop()
        {
            if (!IsRunning) return;
            IsRunning = false;
            _dataTimer.Stop();
        }

        // ----------------------------------------------------------------
        // Empfang vom "PC" -> wir taeuschen das Board
        // ----------------------------------------------------------------
        public void HandleTx(string frame)
        {
            // gleiche Parser-Logik wie auf dem Board
            var parsed = FrameProtocol.Parse(frame.TrimEnd('\r', '\n'));
            if (parsed == null) return;

            if (parsed.Type == "IO")
            {
                // $IO,LT1377,<0|1>  ->  nach kurzer Pause $ACK,LT1377,<0|1>
                string name  = parsed.Field(1);
                string state = parsed.Field(2);
                ScheduleAck(name, state);
            }
            // $PC-Frames werden in der echten Firmware nur fuer das Display benutzt
            // -> hier ignorieren wir sie (kein Feedback noetig)
        }

        private void ScheduleAck(string name, string state)
        {
            var t = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(AckDelayMs) };
            t.Tick += (s, e) =>
            {
                t.Stop();
                string ack = FrameProtocol.Build("ACK", name, state);
                EmitFrame(ack);
            };
            t.Start();
        }

        // ----------------------------------------------------------------
        // Datengenerierung
        // ----------------------------------------------------------------
        private void GenerateDataFrame()
        {
            _phase += 0.05;

            // Random walk + langsame Sinus-Schwingung -> sieht im Chart lebendig aus
            _shtT = Clamp(_shtT + Nudge(0.05) + Math.Sin(_phase)       * 0.02, 18.0, 30.0);
            _shtH = Clamp(_shtH + Nudge(0.15),                                 30.0, 70.0);
            _ufT  = Clamp(_ufT  + Nudge(0.06) + Math.Sin(_phase * 1.3) * 0.02, 20.0, 35.0);
            _vbat = Clamp(_vbat + Nudge(0.002),                                3.20, 3.35);
            _vsup = Clamp(_vsup + Nudge(0.005),                                4.85, 5.10);

            // ganz selten einen kaputten Frame schicken, damit der CRC-Zaehler
            // im UI auch mal hochzaehlt (1 von ~200 Frames)
            if (_rng.Next(200) == 0)
            {
                EmitRaw("$DATA,this,is,garbage*ZZ");
                return;
            }

            string frame = FrameProtocol.Build("DATA",
                _shtT.ToString("F2", CultureInfo.InvariantCulture),
                _shtH.ToString("F2", CultureInfo.InvariantCulture),
                _ufT .ToString("F2", CultureInfo.InvariantCulture),
                _vbat.ToString("F3", CultureInfo.InvariantCulture),
                _vsup.ToString("F3", CultureInfo.InvariantCulture));

            EmitFrame(frame);
        }

        // ----------------------------------------------------------------
        // Helpers
        // ----------------------------------------------------------------
        private void EmitFrame(string frameWithCrLf)
        {
            // CRLF entfernen – die echte RX-Pipeline liefert Zeilen ohne CRLF
            EmitRaw(frameWithCrLf.TrimEnd('\r', '\n'));
        }

        private void EmitRaw(string line)
        {
            if (_emitLine != null) _emitLine(line);
        }

        private double Nudge(double scale)
        {
            return (_rng.NextDouble() - 0.5) * scale * 2.0;
        }

        private static double Clamp(double v, double lo, double hi)
        {
            if (v < lo) return lo;
            if (v > hi) return hi;
            return v;
        }
    }
}
