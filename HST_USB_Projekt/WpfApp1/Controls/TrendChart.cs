using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Media;

namespace WpfApp1.Controls
{
    /// <summary>
    /// Schlankes, selbstgezeichnetes Liniendiagramm (ohne Fremdbibliothek).
    /// Haelt pro Serie einen Ringpuffer und zeichnet scrollend von links nach
    /// rechts. Die Y-Achse skaliert automatisch auf das sichtbare Min/Max.
    /// </summary>
    public sealed class TrendChart : FrameworkElement
    {
        private sealed class Series
        {
            public string Name;
            public Brush Color;
            public readonly List<double> Values = new List<double>();
            public Series(string name, Brush color) { Name = name; Color = color; }
        }

        private readonly List<Series> _series = new List<Series>();
        private readonly int _capacity;

        private readonly Pen _gridPen;
        private readonly Brush _textBrush = new SolidColorBrush(Color.FromRgb(0x8A, 0x8D, 0x93));
        private readonly Typeface _tf = new Typeface("Segoe UI");

        public TrendChart() : this(180) { }

        public TrendChart(int capacity)
        {
            _capacity = capacity;
            _gridPen = new Pen(new SolidColorBrush(Color.FromRgb(0x3A, 0x3E, 0x47)), 1);
            _gridPen.Freeze();
            _textBrush.Freeze();
        }

        public int AddSeries(string name, Color color)
        {
            var b = new SolidColorBrush(color);
            b.Freeze();
            _series.Add(new Series(name, b));
            return _series.Count - 1;
        }

        public void Push(int seriesIndex, double value)
        {
            if (seriesIndex < 0 || seriesIndex >= _series.Count) return;
            var vals = _series[seriesIndex].Values;
            vals.Add(value);
            if (vals.Count > _capacity) vals.RemoveAt(0);
            InvalidateVisual();
        }

        public void Clear()
        {
            foreach (var s in _series) s.Values.Clear();
            InvalidateVisual();
        }

        protected override void OnRender(DrawingContext dc)
        {
            double w = ActualWidth, h = ActualHeight;
            if (w <= 1 || h <= 1) return;

            const double padL = 44, padR = 10, padT = 10, padB = 22;
            double plotW = w - padL - padR;
            double plotH = h - padT - padB;
            if (plotW <= 0 || plotH <= 0) return;

            // ---- gemeinsames Min/Max ueber alle Serien bestimmen ----
            double min = double.MaxValue, max = double.MinValue;
            int maxCount = 0;
            foreach (var s in _series)
            {
                foreach (var v in s.Values)
                {
                    if (v < min) min = v;
                    if (v > max) max = v;
                }
                if (s.Values.Count > maxCount) maxCount = s.Values.Count;
            }
            if (maxCount == 0) { DrawEmpty(dc, padL, padT, plotW, plotH); return; }
            if (min == double.MaxValue) { min = 0; max = 1; }
            if (Math.Abs(max - min) < 0.5) { max += 0.5; min -= 0.5; } // Mindestspanne
            double range = max - min;

            // ---- horizontale Gitterlinien + Y-Beschriftung (5 Stufen) ----
            const int grid = 4;
            for (int i = 0; i <= grid; i++)
            {
                double y = padT + plotH * i / grid;
                dc.DrawLine(_gridPen, new Point(padL, y), new Point(padL + plotW, y));

                double val = max - range * i / grid;
                var ft = MakeText(val.ToString("F1", CultureInfo.InvariantCulture));
                dc.DrawText(ft, new Point(padL - ft.Width - 6, y - ft.Height / 2));
            }

            // ---- Serien als Polylines ----
            foreach (var s in _series)
            {
                int n = s.Values.Count;
                if (n < 2) continue;

                var pen = new Pen(s.Color, 2);
                pen.LineJoin = PenLineJoin.Round;
                pen.StartLineCap = PenLineCap.Round;
                pen.EndLineCap = PenLineCap.Round;
                pen.Freeze();

                var geo = new StreamGeometry();
                using (var ctx = geo.Open())
                {
                    for (int i = 0; i < n; i++)
                    {
                        double x = padL + plotW * i / (_capacity - 1);
                        double y = padT + plotH * (1.0 - (s.Values[i] - min) / range);
                        if (i == 0) ctx.BeginFigure(new Point(x, y), false, false);
                        else ctx.LineTo(new Point(x, y), true, false);
                    }
                }
                geo.Freeze();
                dc.DrawGeometry(null, pen, geo);
            }

            // ---- Legende oben links innerhalb Plot ----
            double lx = padL + 6, ly = padT + 4;
            foreach (var s in _series)
            {
                dc.DrawRectangle(s.Color, null, new Rect(lx, ly + 3, 12, 4));
                var ft = MakeText(s.Name);
                dc.DrawText(ft, new Point(lx + 16, ly - 2));
                lx += 16 + ft.Width + 16;
            }
        }

        private void DrawEmpty(DrawingContext dc, double x, double y, double w, double h)
        {
            var ft = MakeText("warte auf Daten ...");
            dc.DrawText(ft, new Point(x + w / 2 - ft.Width / 2, y + h / 2 - ft.Height / 2));
        }

        private FormattedText MakeText(string s)
        {
            return new FormattedText(
                s, CultureInfo.InvariantCulture, FlowDirection.LeftToRight,
                _tf, 11, _textBrush);
        }
    }
}
