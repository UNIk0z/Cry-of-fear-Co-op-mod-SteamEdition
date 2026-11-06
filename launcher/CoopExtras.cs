// v0.9 launcher pages (Katz's QoL list, docs/COOP_QOL_PROPOSAL.md): SETTINGS (FOV, head bob, co-op
// keys: written to cryoffear\storycoop_comfort.cfg, which the co-op mod runs at every co-op start),
// SPRAY (a picture becomes the game's own spray file tempdecal.wad) and the rejoin offer after the game
// closed abnormally while this PC was joined to a host (cryoffear\storycoop_session.txt, written by the
// co-op mod). C# 5, like CoopLauncher.cs.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace CofCoop
{
    // The engine takes a custom logo of 1024 to 20480 bytes (decomp engine_hw com_custom.c), one miptex
    // of at most 256x256 with four mip levels and a palette of 256 colours (decals.c
    // Draw_ValidateCustomLogo); index 255 is transparent in a "{" texture.
    static class Spray
    {
        public const int MaxPixels = 14336, MinFile = 1024, MaxFile = 20480;

        public static Size FitSize(int width, int height)
        {
            double s = Math.Min(1.0, 128.0 / Math.Max(1, Math.Max(width, height)));
            for (int n = 0; n < 200; n++, s *= 0.95)
            {
                int w = Math.Max(16, (int)(width * s) / 16 * 16), h = Math.Max(16, (int)(height * s) / 16 * 16);
                if (w * h <= MaxPixels) return new Size(w, h);
            }
            return new Size(96, 96);
        }

        public static Bitmap Resize(Image source)
        {
            Size size = FitSize(source.Width, source.Height);
            Bitmap bmp = new Bitmap(size.Width, size.Height, PixelFormat.Format32bppArgb);
            using (Graphics g = Graphics.FromImage(bmp))
            {
                g.Clear(Color.Transparent);
                g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                g.PixelOffsetMode = PixelOffsetMode.HighQuality;
                g.SmoothingMode = SmoothingMode.HighQuality;
                g.DrawImage(source, new Rectangle(0, 0, size.Width, size.Height));
            }
            return bmp;
        }

        static int[] Pixels(Bitmap bmp)
        {
            Rectangle r = new Rectangle(0, 0, bmp.Width, bmp.Height);
            BitmapData data = bmp.LockBits(r, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
            int[] pixels = new int[bmp.Width * bmp.Height];
            for (int y = 0; y < bmp.Height; y++)
                System.Runtime.InteropServices.Marshal.Copy(new IntPtr(data.Scan0.ToInt64() + (long)y * data.Stride), pixels, y * bmp.Width, bmp.Width);
            bmp.UnlockBits(data);
            return pixels;
        }

        // Median cut over the opaque pixels into at most 255 colours; index 255 stays transparent (blue).
        static byte[] Quantize(int[] argb, out byte[] palette)
        {
            palette = new byte[768];
            palette[765] = 0; palette[766] = 0; palette[767] = 255;
            List<int> opaque = new List<int>();
            foreach (int c in argb) if (((c >> 24) & 0xFF) >= 128) opaque.Add(c & 0xFFFFFF);
            byte[] index = new byte[argb.Length];
            if (opaque.Count == 0) { for (int i = 0; i < index.Length; i++) index[i] = 255; return index; }
            List<List<int>> boxes = new List<List<int>>();
            boxes.Add(opaque);
            while (boxes.Count < 255)
            {
                int best = -1, bestRange = 0, bestChannel = 0;
                for (int b = 0; b < boxes.Count; b++)
                {
                    if (boxes[b].Count < 2) continue;
                    for (int ch = 0; ch < 3; ch++)
                    {
                        int lo = 255, hi = 0, shift = 16 - ch * 8;
                        foreach (int c in boxes[b]) { int v = (c >> shift) & 0xFF; if (v < lo) lo = v; if (v > hi) hi = v; }
                        if (hi - lo > bestRange) { bestRange = hi - lo; best = b; bestChannel = ch; }
                    }
                }
                if (best < 0) break;
                int sh = 16 - bestChannel * 8;
                List<int> box = boxes[best];
                box.Sort(delegate(int x, int y) { return ((x >> sh) & 0xFF).CompareTo((y >> sh) & 0xFF); });
                int mid = box.Count / 2;
                boxes[best] = box.GetRange(0, mid);
                boxes.Add(box.GetRange(mid, box.Count - mid));
            }
            for (int b = 0; b < boxes.Count; b++)
            {
                long r = 0, g = 0, bl = 0;
                foreach (int c in boxes[b]) { r += (c >> 16) & 0xFF; g += (c >> 8) & 0xFF; bl += c & 0xFF; }
                int n = boxes[b].Count;
                palette[b * 3] = (byte)(r / n); palette[b * 3 + 1] = (byte)(g / n); palette[b * 3 + 2] = (byte)(bl / n);
            }
            Dictionary<int, byte> cache = new Dictionary<int, byte>();
            for (int i = 0; i < argb.Length; i++)
            {
                int c = argb[i];
                if (((c >> 24) & 0xFF) < 128) { index[i] = 255; continue; }
                int rgb = c & 0xFFFFFF; byte found;
                if (!cache.TryGetValue(rgb, out found))
                {
                    int best = 0, bestDistance = int.MaxValue;
                    for (int p = 0; p < boxes.Count; p++)
                    {
                        int dr = ((rgb >> 16) & 0xFF) - palette[p * 3], dg = ((rgb >> 8) & 0xFF) - palette[p * 3 + 1], db = (rgb & 0xFF) - palette[p * 3 + 2];
                        int d = dr * dr + dg * dg + db * db;
                        if (d < bestDistance) { bestDistance = d; best = p; }
                    }
                    found = (byte)best; cache[rgb] = found;
                }
                index[i] = found;
            }
            return index;
        }

        // tempdecal.wad: WAD3 with one miptex "{LOGO".
        public static byte[] MakeWad(Bitmap resized, out Bitmap preview)
        {
            int w = resized.Width, h = resized.Height;
            byte[] palette;
            byte[] mip0 = Quantize(Pixels(resized), out palette);
            preview = new Bitmap(w, h, PixelFormat.Format32bppArgb);
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    int i = mip0[y * w + x];
                    preview.SetPixel(x, y, i == 255 ? Color.Transparent : Color.FromArgb(palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]));
                }
            MemoryStream lump = new MemoryStream();
            using (BinaryWriter b = new BinaryWriter(lump, Encoding.ASCII, true))
            {
                byte[] name = new byte[16]; Encoding.ASCII.GetBytes("{LOGO").CopyTo(name, 0);
                b.Write(name); b.Write(w); b.Write(h);
                int offset = 40;
                for (int m = 0; m < 4; m++) { b.Write(offset); offset += (w >> m) * (h >> m); }
                for (int m = 0; m < 4; m++)
                    for (int y = 0; y < (h >> m); y++)
                        for (int x = 0; x < (w >> m); x++) b.Write(mip0[(y << m) * w + (x << m)]);
                b.Write((short)256); b.Write(palette); b.Write((short)0);
            }
            byte[] data = lump.ToArray();
            MemoryStream file = new MemoryStream();
            using (BinaryWriter b = new BinaryWriter(file, Encoding.ASCII, true))
            {
                b.Write(Encoding.ASCII.GetBytes("WAD3")); b.Write(1); b.Write(12 + data.Length);
                b.Write(data);
                b.Write(12); b.Write(data.Length); b.Write(data.Length); b.Write((byte)0x43); b.Write((byte)0); b.Write((short)0);
                byte[] name = new byte[16]; Encoding.ASCII.GetBytes("{LOGO").CopyTo(name, 0); b.Write(name);
            }
            return file.ToArray();
        }

        // The engine's own checks, null when the file passes.
        public static string Check(byte[] wad)
        {
            if (wad.Length < MinFile || wad.Length > MaxFile) return "the file must be 1024 to 20480 bytes, not " + wad.Length;
            if (Encoding.ASCII.GetString(wad, 0, 4) != "WAD3") return "not a WAD3 file";
            int lumps = BitConverter.ToInt32(wad, 4), table = BitConverter.ToInt32(wad, 8);
            if (lumps < 1 || table < 12 || table + lumps * 32 > wad.Length) return "bad lump table";
            int pos = BitConverter.ToInt32(wad, table), disk = BitConverter.ToInt32(wad, table + 4);
            if (pos < 12 || pos + disk > table) return "bad lump";
            int w = BitConverter.ToInt32(wad, pos + 16), h = BitConverter.ToInt32(wad, pos + 20);
            if (w < 1 || h < 1 || w > 256 || h > 256) return "bad size";
            int size = w * h;
            for (int m = 0; m < 3; m++)
            {
                if (BitConverter.ToInt32(wad, pos + 24 + m * 4) + size != BitConverter.ToInt32(wad, pos + 28 + m * 4)) return "bad mip offsets";
                size >>= 2;
            }
            int pix = w * h; pix += (pix >> 2) + (pix >> 4) + (pix >> 6);
            int palette = BitConverter.ToUInt16(wad, pos + 40 + pix);
            if (palette > 256) return "bad palette";
            if (BitConverter.ToInt32(wad, pos + 24) + pix + palette * 3 + 2 > disk) return "lump too short";
            return null;
        }

        // Command line, for tests: --make-spray <picture> <out.wad>
        public static int Command(string picture, string output)
        {
            using (Image source = Image.FromFile(picture))
            using (Bitmap resized = Resize(source))
            {
                Bitmap preview;
                byte[] wad = MakeWad(resized, out preview);
                preview.Dispose();
                if (Check(wad) != null) return 2;
                File.WriteAllBytes(output, wad);
                return 0;
            }
        }
    }

    // storycoop_comfort.cfg: cl_fovmultiplier (CoF's FOV: view = FOV * multiplier), head bob (cl_bob,
    // cl_bobup, cl_bobcycle; CoF defaults 0.01, 0.5, 0.8) and the three co-op keys.
    class ComfortSettings
    {
        public int Fov = 90;
        public int Bob = 2;   // 0 off, 1 low, 2 default
        public string Unstuck = "k", Hud = "h", Spray = "t", Ping = "g", Scare = "j";
        public bool HasView;
        // v0.10: colour (sc_color 0 automatic .. 8), black and white (sc_noir), the faint light around you with no torch on
        // (cof_reducedlightcolour1-3, cof_reducedlightradius: client cvars, CoF defaults 0.23 0.23 0.26 and 60).
        public int Colour = 0, Light = 0;   // Light: 0 game, 1 warm, 2 cold, 3 brighter
        public bool Noir, Wide;
        public static readonly string[] Lights = { "0.23 0.23 0.26", "0.30 0.24 0.17", "0.17 0.23 0.32", "0.32 0.32 0.36" };
        public static readonly string[] Colours = { "Automatic", "Red", "Orange", "Yellow", "Green", "Cyan", "Blue", "Purple", "White" };

        public static readonly Regex KeyName = new Regex("^([a-z0-9]|f([1-9]|1[0-2])|mouse[3-5]|ins|del|home|end|pgup|pgdn|kp_[a-z0-9]+)$");

        public static ComfortSettings Read(string path)
        {
            ComfortSettings s = new ComfortSettings();
            if (!File.Exists(path)) return s;
            foreach (string raw in File.ReadAllLines(path))
            {
                string line = raw.Trim();
                Match m = Regex.Match(line, "^bind\\s+\"?([^\"\\s]+)\"?\\s+\"([^\"]*)\"");
                if (m.Success)
                {
                    string key = m.Groups[1].Value.ToLowerInvariant(), what = m.Groups[2].Value;
                    if (what == "sc_unstuck") s.Unstuck = key; else if (what == "sc_hud") s.Hud = key; else if (what == "impulse 201") s.Spray = key;
                    else if (what == "sc_ping") s.Ping = key; else if (what == "sc_scare") s.Scare = key;
                    continue;
                }
                m = Regex.Match(line, "^(cl_fovmultiplier|cl_bob|sc_color|sc_noir|cof_reducedlightcolour1|cof_reducedlightcolour3|cof_reducedlightradius)\\s+\"?([0-9.]+)\"?");
                if (!m.Success) continue;
                double v;
                if (!double.TryParse(m.Groups[2].Value, NumberStyles.Float, CultureInfo.InvariantCulture, out v)) continue;
                string name = m.Groups[1].Value;
                if (name == "cl_fovmultiplier") { s.Fov = (int)Math.Round(Math.Max(75, Math.Min(110, 90 * v))); s.HasView = true; }
                else if (name == "cl_bob") { s.Bob = v <= 0.0001 ? 0 : v < 0.008 ? 1 : 2; s.HasView = true; }
                else if (name == "sc_color") s.Colour = Math.Max(0, Math.Min(8, (int)v));
                else if (name == "sc_noir") s.Noir = v != 0;
                else if (name == "cof_reducedlightradius") s.Wide = v > 75;
                else if (name == "cof_reducedlightcolour1") s.Light = v >= 0.315 ? 3 : v >= 0.28 ? 1 : v <= 0.19 ? 2 : 0;
            }
            return s;
        }

        public string Text()
        {
            StringBuilder b = new StringBuilder();
            b.AppendLine("// Cry of Fear Co-op: your view settings and co-op keys.");
            b.AppendLine("// The co-op mod runs this file once at every co-op start, on this PC only.");
            b.AppendLine("// Written by SETTINGS in \"Cry of Fear Co-op.exe\"; you can also edit it by hand.");
            b.AppendLine("// The game keeps key binds in its own config.cfg when it closes.");
            b.AppendLine();
            b.AppendLine("// Field of view " + Fov + " degrees (the game's FOV multiplier)");
            b.AppendLine("cl_fovmultiplier \"" + (Fov / 90.0).ToString("0.000", CultureInfo.InvariantCulture) + "\"");
            b.AppendLine("// Head bob: " + (Bob == 0 ? "off" : Bob == 1 ? "low" : "default"));
            b.AppendLine("cl_bob \"" + (Bob == 0 ? "0" : Bob == 1 ? "0.004" : "0.01") + "\"");
            b.AppendLine("cl_bobup \"0.5\"");
            b.AppendLine("cl_bobcycle \"0.8\"");
            b.AppendLine();
            b.AppendLine("// Unstuck (moves you out of a wall, or next to your partner)");
            b.AppendLine("bind \"" + Unstuck + "\" \"sc_unstuck\"");
            b.AppendLine("// HUD on/off for screenshots");
            b.AppendLine("bind \"" + Hud + "\" \"sc_hud\"");
            b.AppendLine("// Spray your logo");
            b.AppendLine("bind \"" + Spray + "\" \"impulse 201\"");
            b.AppendLine("// Ping: show the others where you look");
            b.AppendLine("bind \"" + Ping + "\" \"sc_ping\"");
            b.AppendLine("// Scare the nearest teammate with a laugh");
            b.AppendLine("bind \"" + Scare + "\" \"sc_scare\"");
            b.AppendLine();
            b.AppendLine("// Your colour: " + Colours[Colour] + " (0 automatic, 1 red, 2 orange, 3 yellow, 4 green, 5 cyan, 6 blue, 7 purple, 8 white)");
            b.AppendLine("sc_color \"" + Colour + "\"");
            b.AppendLine("// Black and white picture (1 = on)");
            b.AppendLine("sc_noir \"" + (Noir ? "1" : "0") + "\"");
            string[] rgb = Lights[Light].Split(' ');
            b.AppendLine("// Your faint light when no torch is on: " + (Light == 0 ? "the game's" : Light == 1 ? "warm" : Light == 2 ? "cold" : "brighter") + (Wide ? ", wide" : ""));
            b.AppendLine("cof_reducedlightcolour1 \"" + rgb[0] + "\"");
            b.AppendLine("cof_reducedlightcolour2 \"" + rgb[1] + "\"");
            b.AppendLine("cof_reducedlightcolour3 \"" + rgb[2] + "\"");
            b.AppendLine("cof_reducedlightradius \"" + (Wide ? "90" : "60") + "\"");
            return b.ToString();
        }

        // What config.cfg binds to each key now (read only).
        public static Dictionary<string, string> GameBinds(string game)
        {
            Dictionary<string, string> binds = new Dictionary<string, string>();
            string path = Path.Combine(game, "config.cfg");
            if (!File.Exists(path)) return binds;
            try
            {
                foreach (string raw in File.ReadAllLines(path))
                {
                    Match m = Regex.Match(raw.Trim(), "^bind\\s+\"([^\"]+)\"\\s+\"([^\"]*)\"");
                    if (m.Success) binds[m.Groups[1].Value.ToLowerInvariant()] = m.Groups[2].Value;
                }
            }
            catch { }
            return binds;
        }
    }

    // A small themed dialog base: dark, borderless, draggable, with text lines.
    class DarkDialog : Form
    {
        protected readonly float scale;
        protected readonly Font font, small;
        Point dragFrom;

        public DarkDialog(string title, int width, int height)
        {
            using (Graphics g = CreateGraphics()) scale = Math.Max(1f, g.DpiX / 96f);
            Text = title;
            AutoScaleMode = AutoScaleMode.None;
            FormBorderStyle = FormBorderStyle.None;
            StartPosition = FormStartPosition.CenterParent;
            ShowInTaskbar = false;
            BackColor = Theme.Back;
            ForeColor = Theme.Ink;
            ClientSize = new Size(S(width), S(height));
            font = new Font("Consolas", 13f * scale, FontStyle.Regular, GraphicsUnit.Pixel);
            small = new Font("Consolas", 11f * scale, FontStyle.Regular, GraphicsUnit.Pixel);
            Font = font;
            KeyPreview = true;
            KeyDown += delegate(object s, KeyEventArgs e) { if (e.KeyCode == Keys.Escape) Close(); };
        }

        protected int S(float v) { return (int)Math.Round(v * scale); }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            using (Pen p = new Pen(Theme.Line, 1f)) e.Graphics.DrawRectangle(p, 0, 0, ClientSize.Width - 1, ClientSize.Height - 1);
            using (Brush b = new SolidBrush(Theme.Blood)) e.Graphics.DrawString(Text.ToUpperInvariant(), font, b, S(20), S(14));
        }

        protected override void OnMouseDown(MouseEventArgs e) { if (e.Button == MouseButtons.Left) dragFrom = e.Location; base.OnMouseDown(e); }
        protected override void OnMouseMove(MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left) Location = new Point(Location.X + e.X - dragFrom.X, Location.Y + e.Y - dragFrom.Y);
            base.OnMouseMove(e);
        }

        protected Label AddLabel(string text, int x, int y, int w, int h, bool dim)
        {
            Label l = new Label();
            l.Text = text; l.Font = dim ? small : font; l.ForeColor = dim ? Theme.Dim : Theme.Ink; l.BackColor = Color.Transparent;
            l.Location = new Point(S(x), S(y)); l.Size = new Size(S(w), S(h));
            Controls.Add(l);
            return l;
        }

        protected MenuLine AddButton(string text, int x, int y, EventHandler click)
        {
            MenuLine b = new MenuLine(text, "", font);
            b.Location = new Point(S(x), S(y)); b.Size = new Size(S(Math.Max(90, text.Length * 9 + 30)), S(24));
            b.Click += click;
            Controls.Add(b);
            return b;
        }
    }

    class SettingsForm : DarkDialog
    {
        readonly string game, path;
        readonly ComfortSettings settings;
        readonly Dictionary<string, string> binds;
        TrackBar fov;
        Label fovText, note;
        RadioButton[] bob = new RadioButton[3], light = new RadioButton[4];
        CheckBox wide, noir;
        ComboBox colour;
        TextBox[] keys = new TextBox[5];
        Label[] keyNotes = new Label[5];
        static readonly string[] Ours = { "sc_unstuck", "sc_hud", "impulse 201", "sc_ping", "sc_scare" };

        public SettingsForm(string gameDirectory) : base("Settings", 560, 600)
        {
            game = gameDirectory;
            path = Path.Combine(game, "storycoop_comfort.cfg");
            settings = ComfortSettings.Read(path);
            binds = ComfortSettings.GameBinds(game);
            AddLabel("Used at every co-op start on this PC. Saved in cryoffear\\storycoop_comfort.cfg.", 20, 40, 520, 18, true);

            AddLabel("Field of view", 20, 72, 160, 20, false);
            fov = new TrackBar();
            fov.Minimum = 75; fov.Maximum = 110; fov.TickFrequency = 5; fov.Value = Math.Max(75, Math.Min(110, settings.Fov));
            fov.Location = new Point(S(180), S(66)); fov.Size = new Size(S(260), S(30)); fov.BackColor = Theme.Back;
            fovText = AddLabel("", 448, 72, 100, 20, false);
            fov.ValueChanged += delegate { fovText.Text = fov.Value + (fov.Value == 90 ? " (game)" : ""); };
            fovText.Text = fov.Value + (fov.Value == 90 ? " (game)" : "");
            Controls.Add(fov);

            AddLabel("Head bob", 20, 110, 160, 20, false);
            string[] names = { "Off", "Low", "Default" };
            for (int i = 0; i < 3; i++) { bob[i] = Radio(names[i], 180 + i * 100, 108, settings.Bob == i, null); }

            AddLabel("Light, no torch", 20, 144, 160, 20, false);
            string[] lights = { "Game", "Warm", "Cold", "Brighter" };
            Panel lightGroup = new Panel(); lightGroup.Location = new Point(S(180), S(140)); lightGroup.Size = new Size(S(370), S(26)); lightGroup.BackColor = Color.Transparent; Controls.Add(lightGroup);
            for (int i = 0; i < 4; i++) { light[i] = Radio(lights[i], i * 86, 2, settings.Light == i, lightGroup); }
            wide = Check("Wider", 180, 168, settings.Wide);

            AddLabel("Your colour", 20, 202, 160, 20, false);
            colour = new ComboBox(); colour.DropDownStyle = ComboBoxStyle.DropDownList; colour.Font = font; colour.BackColor = Color.FromArgb(0x18, 0x17, 0x16); colour.ForeColor = Theme.Ink; colour.FlatStyle = FlatStyle.Flat;
            colour.Items.AddRange(ComfortSettings.Colours); colour.SelectedIndex = Math.Max(0, Math.Min(8, settings.Colour));
            colour.Location = new Point(S(180), S(198)); colour.Size = new Size(S(130), S(24)); Controls.Add(colour);
            noir = Check("Black and white", 330, 200, settings.Noir);
            AddLabel("Your colour marks your ping and, if the host turns it on, your outline.", 20, 226, 520, 18, true);

            string[] labels = { "Unstuck key", "HUD on/off key", "Spray key", "Ping key", "Scare key" };
            string[] values = { settings.Unstuck, settings.Hud, settings.Spray, settings.Ping, settings.Scare };
            for (int i = 0; i < 5; i++)
            {
                int y = 256 + i * 46;
                AddLabel(labels[i], 20, y + 2, 160, 20, false);
                TextBox t = new TextBox();
                t.Text = values[i].ToUpperInvariant(); t.Font = font; t.BackColor = Color.FromArgb(0x18, 0x17, 0x16); t.ForeColor = Theme.Ink; t.BorderStyle = BorderStyle.FixedSingle;
                t.Location = new Point(S(180), S(y)); t.Size = new Size(S(90), S(24)); t.CharacterCasing = CharacterCasing.Upper; t.MaxLength = 8;
                int k = i;
                t.TextChanged += delegate { ShowKeyNote(k); };
                keys[i] = t; Controls.Add(t);
                keyNotes[i] = AddLabel("", 280, y + 4, 270, 18, true);
                ShowKeyNote(i);
            }
            AddLabel("Keys: a letter, a digit, F1-F12, MOUSE3-5. K is voice chat in a new config.", 20, 488, 520, 18, true);
            note = AddLabel("", 20, 512, 520, 36, false);
            AddButton("SAVE", 20, 560, delegate { Save(); });
            AddButton("CLOSE", 160, 560, delegate { Close(); });
        }

        RadioButton Radio(string text, int x, int y, bool on, Control parent)
        {
            RadioButton r = new RadioButton();
            r.Text = text; r.Font = font; r.ForeColor = Theme.Ink; r.BackColor = Color.Transparent;
            r.Location = new Point(S(x), S(y)); r.Size = new Size(S(84), S(24)); r.Checked = on;
            (parent ?? this).Controls.Add(r);
            return r;
        }

        CheckBox Check(string text, int x, int y, bool on)
        {
            CheckBox c = new CheckBox();
            c.Text = text; c.Font = font; c.ForeColor = Theme.Ink; c.BackColor = Color.Transparent;
            c.Location = new Point(S(x), S(y)); c.Size = new Size(S(200), S(24)); c.Checked = on;
            Controls.Add(c);
            return c;
        }

        void ShowKeyNote(int i)
        {
            string key = keys[i].Text.Trim().ToLowerInvariant();
            string what;
            if (!ComfortSettings.KeyName.IsMatch(key)) keyNotes[i].Text = "not a key name";
            else if (binds.TryGetValue(key, out what) && what != Ours[i]) keyNotes[i].Text = "now in your config.cfg: " + what + " (replaced in co-op)";
            else keyNotes[i].Text = "";
        }

        void Save()
        {
            string[] chosen = new string[5];
            for (int i = 0; i < 5; i++)
            {
                chosen[i] = keys[i].Text.Trim().ToLowerInvariant();
                if (!ComfortSettings.KeyName.IsMatch(chosen[i])) { note.Text = "\"" + keys[i].Text + "\" is not a key name."; return; }
                for (int j = 0; j < i; j++) if (chosen[j] == chosen[i]) { note.Text = "Each key can do one thing only."; return; }
            }
            settings.Fov = fov.Value;
            settings.Bob = bob[0].Checked ? 0 : bob[1].Checked ? 1 : 2;
            settings.Light = light[1].Checked ? 1 : light[2].Checked ? 2 : light[3].Checked ? 3 : 0;
            settings.Wide = wide.Checked; settings.Noir = noir.Checked; settings.Colour = Math.Max(0, colour.SelectedIndex);
            settings.Unstuck = chosen[0]; settings.Hud = chosen[1]; settings.Spray = chosen[2]; settings.Ping = chosen[3]; settings.Scare = chosen[4];
            try
            {
                string temp = path + ".new";
                File.WriteAllText(temp, settings.Text(), Encoding.ASCII);
                if (File.Exists(path)) File.Replace(temp, path, null); else File.Move(temp, path);
                note.Text = "Saved. Used the next time you start the co-op mod.";
            }
            catch (Exception ex) { note.Text = "Could not save: " + ex.Message; }
        }
    }

    class SprayForm : DarkDialog
    {
        readonly string game;
        readonly PictureBox before, after;
        readonly Label note;
        byte[] wad;

        public SprayForm(string gameDirectory) : base("Spray", 520, 420)
        {
            game = gameDirectory;
            AddLabel("Pick a picture (PNG, JPG, BMP or GIF; transparent parts stay clear).", 20, 40, 480, 18, true);
            AddLabel("It becomes your spray (cryoffear\\tempdecal.wad). Spray with the key in SETTINGS (T).", 20, 58, 480, 18, true);
            before = Box(20, 90); after = Box(270, 90);
            AddLabel("Your picture", 20, 326, 230, 18, true);
            AddLabel("Your spray in the game", 270, 326, 230, 18, true);
            note = AddLabel("", 20, 348, 490, 36, false);
            AddButton("CHOOSE PICTURE", 20, 386, delegate { Choose(); });
            AddButton("SAVE AS MY SPRAY", 200, 386, delegate { Save(); });
            AddButton("CLOSE", 400, 386, delegate { Close(); });
        }

        PictureBox Box(int x, int y)
        {
            PictureBox p = new PictureBox();
            p.Location = new Point(S(x), S(y)); p.Size = new Size(S(230), S(230)); p.SizeMode = PictureBoxSizeMode.Zoom;
            p.BackColor = Color.FromArgb(0x30, 0x2E, 0x2B); p.BorderStyle = BorderStyle.FixedSingle;
            Controls.Add(p);
            return p;
        }

        void Choose()
        {
            using (OpenFileDialog d = new OpenFileDialog())
            {
                d.Filter = "Pictures|*.png;*.jpg;*.jpeg;*.bmp;*.gif";
                if (d.ShowDialog(this) != DialogResult.OK) return;
                try
                {
                    using (Image source = Image.FromFile(d.FileName))
                    using (Bitmap resized = Spray.Resize(source))
                    {
                        Bitmap preview;
                        wad = Spray.MakeWad(resized, out preview);
                        string problem = Spray.Check(wad);
                        if (problem != null) { wad = null; note.Text = "This picture does not work as a spray: " + problem + "."; return; }
                        if (before.Image != null) before.Image.Dispose();
                        if (after.Image != null) after.Image.Dispose();
                        before.Image = new Bitmap(source);
                        after.Image = Scaled(preview, 4);
                        preview.Dispose();
                        note.Text = resized.Width + " x " + resized.Height + " pixels, " + wad.Length + " bytes. Click SAVE AS MY SPRAY.";
                    }
                }
                catch (Exception ex) { wad = null; note.Text = "Could not read the picture: " + ex.Message; }
            }
        }

        // Nearest-neighbour enlargement on a checker board, so the game's pixels and the clear parts show.
        static Bitmap Scaled(Bitmap small, int factor)
        {
            Bitmap big = new Bitmap(small.Width * factor, small.Height * factor);
            using (Graphics g = Graphics.FromImage(big))
            {
                for (int y = 0; y < big.Height; y += 8) for (int x = 0; x < big.Width; x += 8)
                    using (Brush b = new SolidBrush(((x + y) / 8) % 2 == 0 ? Color.FromArgb(70, 70, 70) : Color.FromArgb(100, 100, 100))) g.FillRectangle(b, x, y, 8, 8);
                g.InterpolationMode = InterpolationMode.NearestNeighbor; g.PixelOffsetMode = PixelOffsetMode.Half;
                g.DrawImage(small, new Rectangle(0, 0, big.Width, big.Height));
            }
            return big;
        }

        void Save()
        {
            if (wad == null) { note.Text = "Choose a picture first."; return; }
            try
            {
                string target = Path.Combine(game, "tempdecal.wad"), keep = target + ".storycoop-original", temp = target + ".new";
                // The first spray this program replaces is kept once beside it.
                if (File.Exists(target) && !File.Exists(keep)) File.Copy(target, keep);
                File.WriteAllBytes(temp, wad);
                if (File.Exists(target)) File.Replace(temp, target, null); else File.Move(temp, target);
                note.Text = "Saved. Your spray is used the next time you start or join a game.";
            }
            catch (Exception ex) { note.Text = "Could not save: " + ex.Message; }
        }
    }

    // The co-op mod writes cryoffear\storycoop_session.txt: "joining <ip:port>" or "joined <ip:port>"
    // while this game is a joiner, "local" when it runs a map itself, "closed" on a normal quit. A crash
    // leaves joining/joined. A host still up answers the engine's A2S_INFO query.
    static class Rejoin
    {
        static readonly Regex Target = new Regex("^[A-Za-z0-9.\\-]{1,253}:[0-9]{1,5}$");

        public static string Pending(string game, DateTime since)
        {
            try
            {
                string path = Path.Combine(game, "storycoop_session.txt");
                if (!File.Exists(path) || File.GetLastWriteTime(path) < since.AddSeconds(-10)) return null;
                string[] words = File.ReadAllText(path).Trim().Split(' ');
                if (words.Length < 2 || (words[0] != "joined" && words[0] != "joining") || !Target.IsMatch(words[1])) return null;
                return words[1];
            }
            catch { return null; }
        }

        public static void Seen(string game, string target)
        {
            try { File.WriteAllText(Path.Combine(game, "storycoop_session.txt"), "seen " + target + "\n", Encoding.ASCII); } catch { }
        }

        public static bool Answers(string target)
        {
            int colon = target.LastIndexOf(':');
            string host = target.Substring(0, colon);
            int port;
            if (!int.TryParse(target.Substring(colon + 1), out port) || port < 1 || port > 65535) return false;
            IPAddress address;
            try
            {
                if (!IPAddress.TryParse(host, out address))
                {
                    address = null;
                    foreach (IPAddress a in Dns.GetHostAddresses(host)) if (a.AddressFamily == AddressFamily.InterNetwork) { address = a; break; }
                    if (address == null) return false;
                }
            }
            catch { return false; }
            byte[] query = new byte[] { 0xFF, 0xFF, 0xFF, 0xFF, (byte)'T' };
            byte[] text = Encoding.ASCII.GetBytes("Source Engine Query\0");
            byte[] packet = new byte[query.Length + text.Length];
            query.CopyTo(packet, 0); text.CopyTo(packet, query.Length);
            for (int attempt = 0; attempt < 3; attempt++)
            {
                try
                {
                    using (UdpClient udp = new UdpClient(AddressFamily.InterNetwork))
                    {
                        udp.Client.ReceiveTimeout = 1500;
                        IPEndPoint to = new IPEndPoint(address, port);
                        udp.Send(packet, packet.Length, to);
                        IPEndPoint from = new IPEndPoint(IPAddress.Any, 0);
                        byte[] reply = udp.Receive(ref from);
                        if (reply.Length >= 5 && reply[0] == 0xFF && reply[1] == 0xFF && reply[2] == 0xFF && reply[3] == 0xFF) return true;
                    }
                }
                catch { }
            }
            return false;
        }
    }

    class RejoinForm : DarkDialog
    {
        public bool Chosen;

        public RejoinForm(string target) : base("Rejoin", 440, 150)
        {
            AddLabel("The game closed. Rejoin " + target + "?", 20, 50, 400, 40, false);
            AddButton("REJOIN", 20, 106, delegate { Chosen = true; Close(); });
            AddButton("CLOSE", 160, 106, delegate { Close(); });
        }
    }
}
