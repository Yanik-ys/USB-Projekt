using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Text;
using System.Windows.Threading;

namespace WpfApp1.Services
{
    /// <summary>
    /// Kapselt den seriellen Port (USB-CDC = virtueller COM-Port).
    /// - Zeilenweiser Empfang (Frame endet mit CR/LF) -> FrameReceived-Event
    /// - Senden von Frames
    /// - Automatischer Reconnect, wenn der Port verschwindet/wieder auftaucht
    /// - Simulations-Modus: wenn portName == "SIM" wird kein echter Port geoeffnet,
    ///   sondern ein virtueller "Board"-Simulator gestartet.
    ///
    /// Alle Events werden auf den UI-Thread (Dispatcher) marshallt.
    /// </summary>
    public sealed class SerialService : IDisposable
    {
        /// <summary>Reservierter Name fuer den eingebauten Simulator.</summary>
        public const string SimPortName = "SIM";

        private SerialPort _port;
        private SimulatedSerial _sim;
        private readonly StringBuilder _rxLine = new StringBuilder();
        private readonly DispatcherTimer _reconnectTimer;
        private readonly Dispatcher _ui;

        private string _targetPort = "";
        private int _baud = 115200;
        private bool _wantConnected;

        public bool IsConnected
        {
            get
            {
                if (_sim != null && _sim.IsRunning) return true;
                return _port != null && _port.IsOpen;
            }
        }

        /// <summary>Feuert bei jeder vollstaendig empfangenen Zeile (ohne CRLF).</summary>
        public event Action<string> FrameReceived;

        /// <summary>Feuert bei Verbindungsaenderungen (true = verbunden).</summary>
        public event Action<bool> ConnectionChanged;

        /// <summary>Feuert mit einer Status-/Logmeldung.</summary>
        public event Action<string> Status;

        public SerialService(Dispatcher uiDispatcher)
        {
            _ui = uiDispatcher;
            _reconnectTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(2)
            };
            _reconnectTimer.Tick += (s, e) => TryReconnect();
        }

        /// <summary>Liste echter Ports + SIM-Eintrag vorangestellt.</summary>
        public static string[] AvailablePorts()
        {
            var list = new List<string>();
            list.Add(SimPortName);
            list.AddRange(SerialPort.GetPortNames());
            return list.ToArray();
        }

        /// <summary>Verbindet (und merkt sich Wunsch fuer Auto-Reconnect).</summary>
        public void Connect(string portName, int baud)
        {
            _targetPort = portName;
            _baud = baud;
            _wantConnected = true;
            _reconnectTimer.Start();
            OpenPort();
        }

        /// <summary>Trennt manuell und stoppt den Auto-Reconnect.</summary>
        public void Disconnect()
        {
            _wantConnected = false;
            _reconnectTimer.Stop();
            ClosePort();
        }

        private void OpenPort()
        {
            try
            {
                ClosePort();

                // ---------- SIM-Mode ----------
                if (_targetPort == SimPortName)
                {
                    _sim = new SimulatedSerial(
                        _ui,
                        line => Raise(FrameReceived, line),
                        msg  => Raise(Status, msg));
                    _sim.Start();
                    Raise(ConnectionChanged, true);
                    return;
                }

                // ---------- Echter Port ----------
                _port = new SerialPort(_targetPort, _baud, Parity.None, 8, StopBits.One);
                _port.ReadTimeout = 500;
                _port.WriteTimeout = 500;
                _port.DtrEnable = true;   // manche CDC-Stacks brauchen DTR
                _port.RtsEnable = true;
                _port.NewLine = "\n";
                _port.DataReceived += OnDataReceived;
                _port.Open();

                Raise(Status, "Verbunden mit " + _targetPort + " @ " + _baud + " Baud");
                Raise(ConnectionChanged, true);
            }
            catch (Exception ex)
            {
                Raise(Status, "Verbindung fehlgeschlagen: " + ex.Message);
                ClosePort();
            }
        }

        private void ClosePort()
        {
            // SIM stoppen
            if (_sim != null)
            {
                _sim.Stop();
                _sim = null;
                Raise(Status, "Simulation gestoppt");
                Raise(ConnectionChanged, false);
                return;
            }

            // echter Port
            if (_port == null) return;
            try
            {
                _port.DataReceived -= OnDataReceived;
                if (_port.IsOpen) _port.Close();
            }
            catch { /* ignore */ }
            finally
            {
                _port.Dispose();
                _port = null;
                Raise(ConnectionChanged, false);
            }
        }

        private void TryReconnect()
        {
            if (!_wantConnected || IsConnected) return;

            // SIM ist immer "verfuegbar"
            if (_targetPort == SimPortName)
            {
                Raise(Status, "Starte Simulation neu ...");
                OpenPort();
                return;
            }

            // nur versuchen, wenn der Zielport aktuell existiert
            if (Array.IndexOf(SerialPort.GetPortNames(), _targetPort) >= 0)
            {
                Raise(Status, "Versuche Reconnect zu " + _targetPort + " ...");
                OpenPort();
            }
        }

        private void OnDataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                if (_port == null) return;
                string chunk = _port.ReadExisting();
                foreach (char c in chunk)
                {
                    if (c == '\r' || c == '\n')
                    {
                        if (_rxLine.Length > 0)
                        {
                            string line = _rxLine.ToString();
                            _rxLine.Clear();
                            Raise(FrameReceived, line);
                        }
                    }
                    else
                    {
                        _rxLine.Append(c);
                        if (_rxLine.Length > 256) _rxLine.Clear(); // Schutz vor Muell
                    }
                }
            }
            catch (Exception ex)
            {
                Raise(Status, "RX-Fehler: " + ex.Message);
                _ui.BeginInvoke(new Action(ClosePort));
            }
        }

        /// <summary>Sendet einen fertigen Frame-String.</summary>
        public bool Send(string frame)
        {
            // SIM-Mode: dem Simulator weiterreichen
            if (_sim != null && _sim.IsRunning)
            {
                _sim.HandleTx(frame);
                return true;
            }

            try
            {
                if (_port != null && _port.IsOpen)
                {
                    _port.Write(frame);
                    return true;
                }
            }
            catch (Exception ex)
            {
                Raise(Status, "TX-Fehler: " + ex.Message);
                _ui.BeginInvoke(new Action(ClosePort));
            }
            return false;
        }

        // ---- Event-Marshalling auf den UI-Thread ----
        private void Raise(Action<string> ev, string arg)
        {
            if (ev != null) _ui.BeginInvoke(new Action(() => ev(arg)));
        }
        private void Raise(Action<bool> ev, bool arg)
        {
            if (ev != null) _ui.BeginInvoke(new Action(() => ev(arg)));
        }

        public void Dispose()
        {
            _reconnectTimer.Stop();
            ClosePort();
        }
    }
}
