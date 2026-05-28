using System;
using System.Globalization;
using WpfApp1.Protocol;

namespace WpfApp1.Models
{
    /// <summary>Ein vom Board empfangener Messdatensatz (decodierter $DATA-Frame).</summary>
    public sealed class Measurement
    {
        public DateTime Timestamp { get; set; }
        public double ShtTemp { get; set; }      // °C
        public double ShtHumidity { get; set; }  // %
        public double Uf200Temp { get; set; }    // °C
        public double Vbat { get; set; }         // V
        public double Vsupply { get; set; }      // V

        /// <summary>
        /// Erzeugt ein Measurement aus einem geparsten $DATA-Frame.
        /// $DATA,&lt;shtT&gt;,&lt;shtH&gt;,&lt;ufT&gt;,&lt;vbat&gt;,&lt;vsup&gt;
        /// Liefert null, wenn die Felder nicht passen.
        /// </summary>
        public static Measurement FromFrame(ParsedFrame f)
        {
            if (f.Type != "DATA" || f.Fields.Length < 6)
                return null;

            double shtT, shtH, ufT, vbat, vsup;
            if (TryD(f.Field(1), out shtT) &&
                TryD(f.Field(2), out shtH) &&
                TryD(f.Field(3), out ufT) &&
                TryD(f.Field(4), out vbat) &&
                TryD(f.Field(5), out vsup))
            {
                return new Measurement
                {
                    Timestamp = DateTime.Now,
                    ShtTemp = shtT,
                    ShtHumidity = shtH,
                    Uf200Temp = ufT,
                    Vbat = vbat,
                    Vsupply = vsup
                };
            }
            return null;
        }

        private static bool TryD(string s, out double v)
        {
            return double.TryParse(s, NumberStyles.Float, CultureInfo.InvariantCulture, out v);
        }

        /// <summary>CSV-Zeile (Semikolon-getrennt, fuer Excel-DE freundlich).</summary>
        public string ToCsvLine()
        {
            return string.Join(";",
                Timestamp.ToString("yyyy-MM-dd HH:mm:ss.fff", CultureInfo.InvariantCulture),
                ShtTemp.ToString("F2", CultureInfo.InvariantCulture),
                ShtHumidity.ToString("F2", CultureInfo.InvariantCulture),
                Uf200Temp.ToString("F2", CultureInfo.InvariantCulture),
                Vbat.ToString("F3", CultureInfo.InvariantCulture),
                Vsupply.ToString("F3", CultureInfo.InvariantCulture));
        }

        public static string CsvHeader()
        {
            return "Timestamp;SHT_Temp_C;SHT_Humidity_%;UF200_Temp_C;Vbat_V;Vsupply_V";
        }
    }
}
