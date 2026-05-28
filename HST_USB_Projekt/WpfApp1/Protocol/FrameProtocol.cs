using System.Globalization;
using System.Text;

namespace WpfApp1.Protocol
{
    /// <summary>
    /// NMEA-style Frame-Protokoll – exakt gespiegelt zur Firmware (usb_protocol.c).
    ///
    ///   $&lt;TYPE&gt;,&lt;f1&gt;,&lt;f2&gt;,...*&lt;CRC&gt;\r\n
    ///   CRC = XOR aller Zeichen zwischen '$' und '*' (exklusiv), 2-stellig hex.
    /// </summary>
    public static class FrameProtocol
    {
        /// <summary>XOR-Checksumme ueber die Payload (ohne '$' und '*').</summary>
        public static byte Checksum(string payload)
        {
            byte cs = 0;
            foreach (char c in payload)
                cs ^= (byte)c;
            return cs;
        }

        /// <summary>Baut einen vollstaendigen Frame inkl. CRC und CRLF.</summary>
        public static string Build(string type, params string[] fields)
        {
            var sb = new StringBuilder();
            sb.Append(type);
            foreach (var f in fields)
            {
                sb.Append(',');
                sb.Append(f);
            }
            string payload = sb.ToString();
            byte cs = Checksum(payload);
            return "$" + payload + "*" + cs.ToString("X2") + "\r\n";
        }

        /// <summary>Baut einen $PC-Frame: Systemzeit, CPU-Last, CPU-Temp.</summary>
        public static string BuildPc(string timeHms, int cpuLoad, int cpuTemp)
        {
            return Build("PC", timeHms,
                         cpuLoad.ToString(CultureInfo.InvariantCulture),
                         cpuTemp.ToString(CultureInfo.InvariantCulture));
        }

        /// <summary>Baut einen $IO-Frame zum Schalten eines Ausgangs.</summary>
        public static string BuildIo(string name, bool on)
        {
            return Build("IO", name, on ? "1" : "0");
        }

        /// <summary>
        /// Parst eine empfangene Zeile, prueft die Checksumme und zerlegt sie.
        /// Liefert null, wenn der Frame ungueltig ist (Format/Checksumme).
        /// </summary>
        public static ParsedFrame Parse(string line)
        {
            if (string.IsNullOrEmpty(line) || line[0] != '$')
                return null;

            int star = line.IndexOf('*');
            if (star < 0 || star + 2 >= line.Length)
                return null;

            string payload = line.Substring(1, star - 1);
            string crcStr = line.Substring(star + 1, 2);

            byte crcRecv;
            if (!byte.TryParse(crcStr, NumberStyles.HexNumber,
                               CultureInfo.InvariantCulture, out crcRecv))
                return null;

            byte crcCalc = Checksum(payload);
            if (crcRecv != crcCalc)
                return null;   // Checksumme passt nicht -> verwerfen

            string[] parts = payload.Split(',');
            if (parts.Length == 0)
                return null;

            return new ParsedFrame(parts[0], parts);
        }
    }

    /// <summary>Ergebnis eines erfolgreich geparsten Frames.</summary>
    public sealed class ParsedFrame
    {
        public string Type { get; private set; }
        public string[] Fields { get; private set; }   // Fields[0] == Type

        public ParsedFrame(string type, string[] fields)
        {
            Type = type;
            Fields = fields;
        }

        public string Field(int index)
        {
            return (index >= 0 && index < Fields.Length) ? Fields[index] : null;
        }
    }
}
