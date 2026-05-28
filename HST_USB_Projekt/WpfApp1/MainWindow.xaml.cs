using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;
using Microsoft.Win32;
using WpfApp1.Models;
using WpfApp1.Protocol;
using WpfApp1.Services;

namespace WpfApp1
{
    public partial class MainWindow : Window
    {
        private readonly SerialService _serial;
        private readonly CsvLogger _csv = new CsvLogger();
        private readonly PcInfoProvider _pc = new PcInfoProvider();
        private readonly DispatcherTimer _pcTimer;

        // Referenzen auf die dynamisch erzeugten Wert-Felder
        private TextBlock _vShtT, _vShtH, _vUfT, _vVbat, _vVsup;

        private int _seriesSht, _seriesUf;
        private long _framesOk, _framesErr;

        public MainWindow()
        {
            InitializeComponent();

            _serial = new SerialService(Dispatcher);
            _serial.FrameReceived += OnFrameReceived;
            _serial.ConnectionChanged += OnConnectionChanged;
            _serial.Status += s => StatusText.Text = s;

            BuildValueRows();
            _seriesSht = Chart.AddSeries("SHT30 °C", Color.FromRgb(0x2D, 0xD4, 0xF0)); // cyan
            _seriesUf  = Chart.AddSeries("UF200 °C", Color.FromRgb(0xF3, 0x9C, 0x12)); // amber

            RefreshPorts();

            // Timer: PC-Infos (Zeit/CPU) einmal pro Sekunde ans Board senden
            _pcTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
            _pcTimer.Tick += (s, e) => SendPcInfo();
            _pcTimer.Start();

            Closed += (s, e) =>
            {
                _serial.Dispose();
                _csv.Dispose();
                _pc.Dispose();
            };
        }

        // ----------------------------------------------------------------
        // UI-Aufbau
        // ----------------------------------------------------------------
        private void BuildValueRows()
        {
            _vShtT = AddValueRow("SHT30 Temperatur", "°C");
            _vShtH = AddValueRow("SHT30 Feuchte",    "%");
            _vUfT  = AddValueRow("UF200 Temperatur", "°C");
            _vVbat = AddValueRow("Batterie / 3V3",   "V");
            _vVsup = AddValueRow("UF200 Versorgung", "V");
        }

        private TextBlock AddValueRow(string label, string unit)
        {
            var grid = new Grid { Margin = new Thickness(0, 0, 0, 10) };
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });

            var lbl = new TextBlock
            {
                Text = label,
                Foreground = (Brush)FindResource("TextMuted"),
                FontSize = 13,
                VerticalAlignment = VerticalAlignment.Center
            };
            Grid.SetColumn(lbl, 0);

            var valPanel = new StackPanel { Orientation = Orientation.Horizontal };
            var val = new TextBlock
            {
                Text = "-",
                FontSize = 19,
                FontWeight = FontWeights.Bold,
                Foreground = (Brush)FindResource("TextBrush")
            };
            var u = new TextBlock
            {
                Text = " " + unit,
                FontSize = 13,
                Foreground = (Brush)FindResource("TextMuted"),
                VerticalAlignment = VerticalAlignment.Bottom,
                Margin = new Thickness(2, 0, 0, 2)
            };
            valPanel.Children.Add(val);
            valPanel.Children.Add(u);
            Grid.SetColumn(valPanel, 1);

            grid.Children.Add(lbl);
            grid.Children.Add(valPanel);
            ValueList.Children.Add(grid);
            return val;
        }

        // ----------------------------------------------------------------
        // Empfang
        // ----------------------------------------------------------------
        private void OnFrameReceived(string line)
        {
            ParsedFrame frame = FrameProtocol.Parse(line);
            if (frame == null)
            {
                _framesErr++;
                FramesErr.Text = _framesErr.ToString();
                return;   // ungueltige Checksumme / Format -> verwerfen
            }

            _framesOk++;
            FramesOk.Text = _framesOk.ToString();

            switch (frame.Type)
            {
                case "DATA": HandleData(frame); break;
                case "ACK":  HandleAck(frame);  break;
            }
        }

        private void HandleData(ParsedFrame frame)
        {
            Measurement m = Measurement.FromFrame(frame);
            if (m == null) return;

            _vShtT.Text = m.ShtTemp.ToString("F2");
            _vShtH.Text = m.ShtHumidity.ToString("F2");
            _vUfT .Text = m.Uf200Temp.ToString("F2");
            _vVbat.Text = m.Vbat.ToString("F3");
            _vVsup.Text = m.Vsupply.ToString("F3");

            Chart.Push(_seriesSht, m.ShtTemp);
            Chart.Push(_seriesUf , m.Uf200Temp);

            if (_csv.IsLogging)
            {
                _csv.Append(m);
                LogCount.Text = _csv.LineCount + " Zeilen";
            }
        }

        private void HandleAck(ParsedFrame frame)
        {
            // $ACK,LT1377,<0|1>
            if (frame.Field(1) == "LT1377")
            {
                bool on = frame.Field(2) == "1";
                Lt1377Led.Fill = (Brush)FindResource(on ? "AccentGreen" : "AccentRed");
                Lt1377State.Text = on ? "EIN (bestaetigt)" : "AUS (bestaetigt)";
                Lt1377State.Foreground = (Brush)FindResource("TextBrush");
            }
        }

        // ----------------------------------------------------------------
        // Senden
        // ----------------------------------------------------------------
        private void SendPcInfo()
        {
            if (!_serial.IsConnected || SendPcChk.IsChecked != true) return;

            string time = _pc.TimeHms;
            int load = _pc.CpuLoad;
            int temp = _pc.CpuTemp;

            string frame = FrameProtocol.BuildPc(time, load < 0 ? 0 : load, temp);
            _serial.Send(frame);

            PcPreview.Text = frame.TrimEnd('\r', '\n');
        }

        // ----------------------------------------------------------------
        // Buttons / Events
        // ----------------------------------------------------------------
        private void OnConnectClick(object sender, RoutedEventArgs e)
        {
            if (_serial.IsConnected)
            {
                _serial.Disconnect();
            }
            else
            {
                string port = PortCombo.SelectedItem as string;
                if (!string.IsNullOrEmpty(port))
                    _serial.Connect(port, 115200);
                else
                    StatusText.Text = "kein Port gewaehlt";
            }
        }

        private void OnConnectionChanged(bool connected)
        {
            StatusLed.Fill = (Brush)FindResource(connected ? "AccentGreen" : "AccentRed");
            StatusText.Text = connected ? "verbunden" : "getrennt";
            ConnectBtn.Content = connected ? "Trennen" : "Verbinden";
            PortCombo.IsEnabled = !connected;
        }

        private void OnRefreshPorts(object sender, RoutedEventArgs e) { RefreshPorts(); }

        private void RefreshPorts()
        {
            string prev = PortCombo.SelectedItem as string;
            PortCombo.ItemsSource = SerialService.AvailablePorts();
            if (prev != null && PortCombo.Items.Contains(prev))
                PortCombo.SelectedItem = prev;
            else if (PortCombo.Items.Count > 0)
                PortCombo.SelectedIndex = 0;
        }

        private void OnLt1377On(object sender, RoutedEventArgs e)
        {
            _serial.Send(FrameProtocol.BuildIo("LT1377", true));
        }

        private void OnLt1377Off(object sender, RoutedEventArgs e)
        {
            _serial.Send(FrameProtocol.BuildIo("LT1377", false));
        }

        private void OnToggleLogging(object sender, RoutedEventArgs e)
        {
            if (_csv.IsLogging)
            {
                _csv.Stop();
                LogBtn.Content = "Logging starten";
                LogBtn.Background = (Brush)FindResource("AccentBlue");
            }
            else
            {
                var dlg = new SaveFileDialog
                {
                    Filter = "CSV-Datei (*.csv)|*.csv",
                    FileName = "messung_" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".csv"
                };
                if (dlg.ShowDialog() == true)
                {
                    _csv.Start(dlg.FileName);
                    LogBtn.Content = "Logging stoppen";
                    LogBtn.Background = (Brush)FindResource("AccentRed");
                    LogPath.Text = System.IO.Path.GetFileName(dlg.FileName);
                    LogCount.Text = "0 Zeilen";
                }
            }
        }

        private void OnSendPcToggled(object sender, RoutedEventArgs e)
        {
            if (SendPcChk.IsChecked != true) PcPreview.Text = "-";
        }
    }
}
