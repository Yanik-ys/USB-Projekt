using System;
using System.Diagnostics;
using System.Globalization;
using System.Management;

namespace WpfApp1.Services
{
    /// <summary>
    /// Liefert die PC-Informationen, die ans Board geschickt werden:
    /// Systemzeit, CPU-Last (%) und – wenn moeglich – CPU-Temperatur (°C).
    ///
    /// Hinweis CPU-Temp: Windows gibt die CPU-Temperatur nur ueber WMI
    /// (MSAcpi_ThermalZoneTemperature) preis, und das auch nur bei manchen
    /// Mainboards und meist mit Adminrechten. Ist sie nicht verfuegbar,
    /// wird -1 zurueckgegeben (das Board zeigt dann "--").
    /// </summary>
    public sealed class PcInfoProvider : IDisposable
    {
        private readonly PerformanceCounter _cpuCounter;
        private int _cpuTempCache = -1;
        private DateTime _cpuTempStamp = DateTime.MinValue;

        public PcInfoProvider()
        {
            try
            {
                _cpuCounter = new PerformanceCounter("Processor", "% Processor Time", "_Total");
                _cpuCounter.NextValue(); // erster Wert ist immer 0 -> einmal "verwerfen"
            }
            catch
            {
                _cpuCounter = null;
            }
        }

        public string TimeHms
        {
            get { return DateTime.Now.ToString("HH:mm:ss", CultureInfo.InvariantCulture); }
        }

        public int CpuLoad
        {
            get
            {
                try
                {
                    if (_cpuCounter == null) return -1;
                    return (int)Math.Round(_cpuCounter.NextValue());
                }
                catch { return -1; }
            }
        }

        public int CpuTemp { get { return CachedCpuTemp(); } }

        /// <summary>
        /// WMI-Abfrage ist langsam (~50-200 ms). Da sie sonst im Sekundentakt
        /// den UI-Thread blockieren wuerde, wird der Wert max. alle 5 s neu gelesen.
        /// </summary>
        private int CachedCpuTemp()
        {
            if ((DateTime.Now - _cpuTempStamp).TotalSeconds >= 5)
            {
                _cpuTempCache = ReadCpuTempWmi();
                _cpuTempStamp = DateTime.Now;
            }
            return _cpuTempCache;
        }

        private static int ReadCpuTempWmi()
        {
            try
            {
                using (var searcher = new ManagementObjectSearcher(
                    @"root\WMI",
                    "SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature"))
                {
                    foreach (ManagementObject obj in searcher.Get())
                    {
                        // Wert ist in Zehntel-Kelvin
                        double tenthsKelvin = Convert.ToDouble(obj["CurrentTemperature"]);
                        double celsius = (tenthsKelvin / 10.0) - 273.15;
                        return (int)Math.Round(celsius);
                    }
                }
            }
            catch
            {
                // WMI nicht verfuegbar / keine Rechte -> still ignorieren
            }
            return -1;
        }

        public void Dispose()
        {
            if (_cpuCounter != null) _cpuCounter.Dispose();
        }
    }
}
