// Cry of Fear Co-op Steam Edition launcher (Katz, 2026-10-30): one window
// instead of Install.cmd, Uninstall.cmd, Run Co-op Mod.cmd and Check for
// Updates.cmd. Hosting and joining stay in the game's own co-op menu.
// Install, Uninstall and Run start the same files\*.ps1 the .cmd files start,
// with the same arguments, so what they do does not change; their output is
// read here (no console window) and "##cof-progress N text" lines from
// find-cof.ps1 Write-CofStep drive the progress bar. The update check follows
// update-coop.ps1 (GitHub's latest release, only a .zip attached to this
// project's own release); Katz chose that an update also unpacks next to this
// folder and installs itself. Built with the .NET Framework csc (C# 5), see
// build-launcher.ps1; no packer, no obfuscation.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.Compression;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using Microsoft.Win32;

namespace CofCoop
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new LauncherForm());
        }
    }

    static class Theme
    {
        public static readonly Color Ink = Color.FromArgb(0xC9, 0xC3, 0xB6);
        public static readonly Color Dim = Color.FromArgb(0x7A, 0x75, 0x6C);
        public static readonly Color Off = Color.FromArgb(0x45, 0x43, 0x40);
        public static readonly Color Blood = Color.FromArgb(0xB3, 0x1B, 0x1B);
        public static readonly Color DarkBlood = Color.FromArgb(0x4A, 0x08, 0x08);
        public static readonly Color Back = Color.FromArgb(0x0C, 0x0C, 0x0C);
        public static readonly Color Line = Color.FromArgb(0x2C, 0x2A, 0x28);
    }

    // A main-menu line like the game's own: plain text, red on hover.
    class MenuLine : Control
    {
        public string Hint;
        public bool Marker = true;
        bool hover;
        int flicker;
        readonly Timer flickerTimer = new Timer();

        public MenuLine(string text, string hint, Font font)
        {
            Text = text; Hint = hint; Font = font;
            SetStyle(ControlStyles.SupportsTransparentBackColor | ControlStyles.OptimizedDoubleBuffer | ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.ResizeRedraw, true);
            BackColor = Color.Transparent;
            Cursor = Cursors.Hand;
            flickerTimer.Interval = 45;
            flickerTimer.Tick += delegate { if (--flicker <= 0) flickerTimer.Stop(); Invalidate(); };
        }

        protected override void OnMouseEnter(EventArgs e) { hover = true; flicker = 5; flickerTimer.Start(); Invalidate(); base.OnMouseEnter(e); }
        protected override void OnMouseLeave(EventArgs e) { hover = false; Invalidate(); base.OnMouseLeave(e); }
        protected override void OnEnabledChanged(EventArgs e) { Cursor = Enabled ? Cursors.Hand : Cursors.Default; Invalidate(); base.OnEnabledChanged(e); }
        protected override void OnTextChanged(EventArgs e) { Invalidate(); base.OnTextChanged(e); }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            bool lit = hover && Enabled && (flicker % 2 == 0);
            Color color = !Enabled ? Theme.Off : lit ? Theme.Blood : hover ? Theme.DarkBlood : Theme.Ink;
            string text = Marker ? (hover && Enabled ? "> " : "  ") + Text : Text;
            if (lit) using (Brush glow = new SolidBrush(Color.FromArgb(90, Theme.DarkBlood))) g.DrawString(text, Font, glow, 1, 2);
            using (Brush b = new SolidBrush(color)) g.DrawString(text, Font, b, 0, 0);
        }
    }


    class LauncherForm : Form
    {
        readonly string baseDir = AppDomain.CurrentDomain.BaseDirectory.TrimEnd('\\');
        readonly string filesDir;
        readonly float scale;
        readonly Font menuFont, smallFont, tinyFont;
        readonly List<MenuLine> scriptLines = new List<MenuLine>();
        MenuLine runLine, updateLine, installLine, uninstallLine, readMeLine, quitLine;
        string infoText = "", messageText = "", hintText = "";
        string version = "";
        Dictionary<string, object> catalog;
        Process working;
        string workingKind;
        List<string> output = new List<string>();
        bool gameRunning, busy;
        Dictionary<string, object> newerAsset;
        string newerTag, releasePage, updateRoot;
        // Progress bar: -1 = hidden, otherwise 0..100; sweep while no percent is known.
        int progress = -1, sweep;
        bool progressKnown, progressFailed;
        readonly Timer gameTimer = new Timer(), sweepTimer = new Timer();
        Point dragFrom;

        const string Title = "Cry of Fear Co-op Steam Edition";
        const string ExeName = "Cry of Fear Co-op.exe";
        const string ReleaseApi = "https://api.github.com/repos/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases/latest";
        const string ReleasesPage = "https://github.com/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases";
        const string DownloadPrefix = "https://github.com/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases/download/";
        const int StatusTop = 372;

        int S(float v) { return (int)Math.Round(v * scale); }

        public LauncherForm()
        {
            filesDir = Path.Combine(baseDir, "files");
            using (Graphics g = CreateGraphics()) scale = Math.Max(1f, g.DpiX / 96f);
            Text = Title;
            AutoScaleMode = AutoScaleMode.None;
            FormBorderStyle = FormBorderStyle.None;
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(S(600), S(470));
            BackColor = Theme.Back;
            DoubleBuffered = true;
            try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }
            menuFont = new Font("Consolas", 15f * scale, FontStyle.Regular, GraphicsUnit.Pixel);
            smallFont = new Font("Consolas", 12f * scale, FontStyle.Regular, GraphicsUnit.Pixel);
            tinyFont = new Font("Consolas", 11f * scale, FontStyle.Regular, GraphicsUnit.Pixel);

            ReadCatalog();
            BackgroundImage = MakeBackground(ClientSize.Width, ClientSize.Height);
            BackgroundImageLayout = ImageLayout.None;

            int y = S(150), step = S(34), x = S(30);
            runLine = AddLine("RUN CO-OP MOD", "Starts Cry of Fear with the co-op main menu. Host or join from the game's menu.", x, ref y, step, true);
            updateLine = AddLine("CHECK FOR UPDATES", "Asks GitHub if a newer version is out. Nothing is changed.", x, ref y, step, false);
            installLine = AddLine("INSTALL", "Installs this version into the game. Your saves stay. Close the game first.", x, ref y, step, true);
            uninstallLine = AddLine("UNINSTALL", "Removes the co-op mod. Saves and your playermodels folder stay.", x, ref y, step, true);
            readMeLine = AddLine("READ ME", "Opens the instructions.", x, ref y, step, false);
            quitLine = AddLine("QUIT", "Closes this window.", x, ref y, step, false);

            runLine.Click += delegate { RunScript(filesDir, "host-story-coop.ps1", " -MenuOnly", "run", "Starting Cry of Fear..."); };
            updateLine.Click += delegate { UpdateClicked(); };
            installLine.Click += delegate { RunScript(filesDir, "setup-coop.ps1", "", "install", "Installing..."); };
            uninstallLine.Click += delegate { Uninstall(); };
            readMeLine.Click += delegate { OpenReadMe(); };
            quitLine.Click += delegate { Close(); };

            AddCaptionButton("x", S(600 - 34), delegate { Close(); });
            AddCaptionButton("_", S(600 - 64), delegate { WindowState = FormWindowState.Minimized; });

            sweepTimer.Interval = 30;
            sweepTimer.Tick += delegate { sweep = (sweep + 2) % 100; Invalidate(StatusArea()); };
            gameTimer.Interval = 1500;
            gameTimer.Tick += delegate { CheckGame(false); };
            gameTimer.Start();
            RefreshInstallState();
            CheckGame(true);
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            // A script still running would be cut off half way.
            if (working != null && e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                Say("Please wait until this step is finished.");
                return;
            }
            base.OnFormClosing(e);
        }

        MenuLine AddLine(string text, string hint, int x, ref int y, int step, bool needsScripts)
        {
            MenuLine line = new MenuLine(text, hint, menuFont);
            line.Location = new Point(x, y);
            line.Size = new Size(S(260), S(26));
            line.MouseEnter += delegate { hintText = line.Hint; Invalidate(StatusArea()); };
            line.MouseLeave += delegate { hintText = ""; Invalidate(StatusArea()); };
            Controls.Add(line);
            if (needsScripts) scriptLines.Add(line);
            y += step;
            return line;
        }

        void AddCaptionButton(string text, int x, EventHandler click)
        {
            MenuLine b = new MenuLine(text, "", smallFont);
            b.Location = new Point(x, S(8));
            b.Size = new Size(S(28), S(22));
            b.Marker = false;
            b.Click += click;
            Controls.Add(b);
        }

        Rectangle StatusArea() { return new Rectangle(0, S(StatusTop + 3), ClientSize.Width, ClientSize.Height - S(StatusTop + 3)); }

        // ---- look ----

        Bitmap MakeBackground(int w, int h)
        {
            Bitmap bmp = new Bitmap(w, h);
            Random rnd = new Random(1985);
            using (Graphics g = Graphics.FromImage(bmp))
            {
                g.Clear(Theme.Back);
                // grain
                for (int i = 0; i < w * h / 3; i++)
                {
                    int v = rnd.Next(4, 30);
                    bmp.SetPixel(rnd.Next(w), rnd.Next(h), Color.FromArgb(v, v - (v > 20 ? 2 : 0), v - (v > 20 ? 3 : 0)));
                }
                g.SmoothingMode = SmoothingMode.AntiAlias;
                // a few worn scratches
                for (int i = 0; i < 14; i++)
                {
                    using (Pen p = new Pen(Color.FromArgb(rnd.Next(10, 26), 200, 190, 170), 1f))
                    {
                        int sx = rnd.Next(w), sy = rnd.Next(h);
                        g.DrawLine(p, sx, sy, sx + rnd.Next(-60, 60), sy + rnd.Next(10, 90));
                    }
                }
                // blood-dark stain behind the menu
                using (GraphicsPath path = new GraphicsPath())
                {
                    path.AddEllipse(-w / 3, h / 4, w, h);
                    using (PathGradientBrush b = new PathGradientBrush(path))
                    {
                        b.CenterColor = Color.FromArgb(46, 70, 0, 0);
                        b.SurroundColors = new Color[] { Color.FromArgb(0, 0, 0, 0) };
                        g.FillPath(b, path);
                    }
                }
                // CRT scanlines
                using (Pen p = new Pen(Color.FromArgb(55, 0, 0, 0), 1f))
                    for (int yy = 0; yy < h; yy += 3) g.DrawLine(p, 0, yy, w, yy);
                // vignette
                using (GraphicsPath path = new GraphicsPath())
                {
                    path.AddEllipse(-w / 4, -h / 4, w * 3 / 2, h * 3 / 2);
                    using (PathGradientBrush b = new PathGradientBrush(path))
                    {
                        b.CenterColor = Color.FromArgb(0, 0, 0, 0);
                        b.SurroundColors = new Color[] { Color.FromArgb(235, 0, 0, 0) };
                        b.FocusScales = new PointF(0.45f, 0.35f);
                        g.FillRectangle(b, 0, 0, w, h);
                    }
                }
                DrawTitle(g, rnd);
                using (Pen p = new Pen(Theme.Line, 1f))
                {
                    g.DrawRectangle(p, 0, 0, w - 1, h - 1);
                    g.DrawLine(p, S(24), S(StatusTop), w - S(24), S(StatusTop));
                }
            }
            return bmp;
        }

        void DrawTitle(Graphics g, Random rnd)
        {
            // Worn off-white letters: the text is filled with a speckled texture.
            using (Bitmap tex = new Bitmap(64, 64))
            {
                for (int yy = 0; yy < 64; yy++)
                    for (int xx = 0; xx < 64; xx++)
                    {
                        int r = rnd.Next(100);
                        tex.SetPixel(xx, yy, r < 12 ? Color.FromArgb(0, 0, 0, 0) : r < 30 ? Color.FromArgb(150, 0x9A, 0x94, 0x88) : Theme.Ink);
                    }
                using (TextureBrush brush = new TextureBrush(tex))
                using (GraphicsPath path = new GraphicsPath())
                using (FontFamily family = new FontFamily("Georgia"))
                {
                    path.AddString("CRY OF FEAR", family, (int)FontStyle.Regular, S(50), new Point(S(28), S(34)), StringFormat.GenericTypographic);
                    using (Matrix m = new Matrix()) { m.Translate(S(2), S(3)); using (GraphicsPath shadow = (GraphicsPath)path.Clone()) { shadow.Transform(m); using (Brush sb = new SolidBrush(Color.FromArgb(150, 60, 0, 0))) g.FillPath(sb, shadow); } }
                    g.FillPath(brush, path);
                }
            }
            g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            const string edition = "CO-OP  STEAM EDITION";
            using (Brush b = new SolidBrush(Theme.Blood))
                g.DrawString(edition, smallFont, b, S(32), S(96));
            using (Brush b = new SolidBrush(Theme.Dim))
                g.DrawString(version, smallFont, b, S(32) + (int)g.MeasureString(edition, smallFont).Width + S(10), S(96));
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            Graphics g = e.Graphics;
            g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            int top = S(StatusTop);
            int left = S(28), width = ClientSize.Width - S(56);
            using (Brush ink = new SolidBrush(Theme.Ink))
            using (Brush dim = new SolidBrush(Theme.Dim))
            {
                g.DrawString(infoText, tinyFont, dim, new RectangleF(left, top + S(8), width, S(16)));
                g.DrawString(hintText.Length > 0 ? hintText : messageText, smallFont, ink, new RectangleF(left, top + S(46), width, S(44)));
            }
            if (progress >= 0) DrawProgress(g, new Rectangle(left, top + S(30), width, S(8)));
        }

        void DrawProgress(Graphics g, Rectangle r)
        {
            g.SmoothingMode = SmoothingMode.None;
            using (Brush back = new SolidBrush(Color.FromArgb(0x10, 0x0F, 0x0E))) g.FillRectangle(back, r);
            Rectangle fill;
            if (progressKnown) fill = new Rectangle(r.X + 1, r.Y + 1, (r.Width - 2) * Math.Min(100, progress) / 100, r.Height - 2);
            else
            {
                int w = r.Width / 4, x = r.X + 1 + (r.Width - 2 + w) * sweep / 100 - w;
                fill = Rectangle.Intersect(new Rectangle(x, r.Y + 1, w, r.Height - 2), new Rectangle(r.X + 1, r.Y + 1, r.Width - 2, r.Height - 2));
            }
            if (fill.Width > 0)
            {
                using (Brush b = new SolidBrush(progressFailed ? Theme.DarkBlood : Theme.Blood)) g.FillRectangle(b, fill);
                using (Pen stripe = new Pen(Color.FromArgb(70, 0, 0, 0), 1f))
                    for (int x = fill.X + 2; x < fill.Right; x += 4) g.DrawLine(stripe, x, fill.Y, x, fill.Bottom - 1);
            }
            using (Pen frame = new Pen(Theme.Line, 1f)) g.DrawRectangle(frame, r.X, r.Y, r.Width - 1, r.Height - 1);
        }

        protected override void OnMouseDown(MouseEventArgs e) { if (e.Button == MouseButtons.Left) dragFrom = e.Location; base.OnMouseDown(e); }
        protected override void OnMouseMove(MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left) Location = new Point(Location.X + e.X - dragFrom.X, Location.Y + e.Y - dragFrom.Y);
            base.OnMouseMove(e);
        }

        void Say(string text) { messageText = text; Invalidate(StatusArea()); }

        void ShowProgress(int percent, bool known)
        {
            progress = Math.Max(0, Math.Min(100, percent)); progressKnown = known; progressFailed = false;
            if (known) sweepTimer.Stop(); else sweepTimer.Start();
            Invalidate(StatusArea());
        }

        void EndProgress(bool ok)
        {
            sweepTimer.Stop();
            progressKnown = true; progressFailed = !ok;
            if (ok || progress < 0) progress = 100;
            Invalidate(StatusArea());
        }

        // ---- state ----

        void ReadCatalog()
        {
            catalog = ReadRelease(filesDir);
            version = catalog == null ? "" : Str(Get(catalog, "coopFixes", "id"));
        }

        static Dictionary<string, object> ReadRelease(string files)
        {
            try { return new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(File.ReadAllText(Path.Combine(files, "release.json"))); }
            catch { return null; }
        }

        static object Get(object o, params string[] keys)
        {
            foreach (string k in keys)
            {
                IDictionary d = o as IDictionary;
                if (d == null || !d.Contains(k)) return null;
                o = d[k];
            }
            return o;
        }
        static string Str(object o) { return o == null ? "" : Convert.ToString(o); }

        bool ScriptsPresent()
        {
            return catalog != null && File.Exists(Path.Combine(filesDir, "setup-coop.ps1")) && File.Exists(Path.Combine(filesDir, "host-story-coop.ps1"));
        }

        void RefreshInstallState()
        {
            if (!ScriptsPresent())
            {
                infoText = "The \"files\" folder next to this program is missing or incomplete.";
                Say("Extract the whole ZIP again, then start this program from that folder.");
                return;
            }
            string game = FindGameDirectory();
            if (game == null) { infoText = "Cry of Fear was not found in your Steam libraries."; Invalidate(StatusArea()); return; }
            string dll = Path.Combine(game, "cl_dlls\\storycoop_rc4.dll");
            string state;
            if (!File.Exists(dll)) state = "co-op not installed yet";
            else
            {
                string sha = "";
                try { using (SHA256 h = SHA256.Create()) using (FileStream f = File.OpenRead(dll)) sha = Hex(h.ComputeHash(f)); } catch { }
                state = sha == Str(Get(catalog, "runtimeAdapter", "sha256")) ? "co-op " + version + " installed" : "a different co-op version is installed";
            }
            infoText = "Game: " + Shorten(Path.GetDirectoryName(game.TrimEnd('\\')), 46) + "  |  " + state;
            Invalidate(StatusArea());
        }

        static string Hex(byte[] bytes) { return BitConverter.ToString(bytes).Replace("-", "").ToLowerInvariant(); }
        static string Shorten(string s, int max) { return s.Length <= max ? s : "..." + s.Substring(s.Length - max + 3); }

        // find-cof.ps1 Find-CofGameDirectory: Steam's libraries, the one with
        // Cry of Fear's appmanifest first.
        static string FindGameDirectory()
        {
            List<string> roots = new List<string>();
            AddRoot(roots, Registry.CurrentUser, "Software\\Valve\\Steam", "SteamPath");
            AddRoot(roots, Registry.LocalMachine, "SOFTWARE\\WOW6432Node\\Valve\\Steam", "InstallPath");
            AddRoot(roots, Registry.LocalMachine, "SOFTWARE\\Valve\\Steam", "InstallPath");
            List<string> libraries = new List<string>(roots);
            foreach (string root in roots)
            {
                try
                {
                    string vdf = Path.Combine(root, "steamapps\\libraryfolders.vdf");
                    if (!File.Exists(vdf)) continue;
                    foreach (Match m in Regex.Matches(File.ReadAllText(vdf), "\"path\"\\s+\"([^\"]+)\""))
                        libraries.Add(m.Groups[1].Value.Replace("\\\\", "\\"));
                }
                catch { }
            }
            string first = null;
            foreach (string library in libraries)
            {
                string candidate = Path.Combine(library, "steamapps\\common\\Cry of Fear\\cryoffear");
                if (!File.Exists(Path.Combine(candidate, "cl_dlls\\hl.dll"))) continue;
                if (File.Exists(Path.Combine(library, "steamapps\\appmanifest_223710.acf"))) return Path.GetFullPath(candidate);
                if (first == null) first = Path.GetFullPath(candidate);
            }
            return first;
        }

        static void AddRoot(List<string> roots, RegistryKey hive, string key, string name)
        {
            try
            {
                using (RegistryKey k = hive.OpenSubKey(key))
                {
                    string v = k == null ? null : k.GetValue(name) as string;
                    if (!string.IsNullOrEmpty(v) && !roots.Contains(v.Replace('/', '\\'))) roots.Add(v.Replace('/', '\\'));
                }
            }
            catch { }
        }

        static bool IsGameRunning()
        {
            foreach (string name in new string[] { "cof", "cryoffear", "hl" })
            {
                Process[] list = Process.GetProcessesByName(name);
                bool any = list.Length > 0;
                foreach (Process p in list) p.Dispose();
                if (any) return true;
            }
            return false;
        }

        void CheckGame(bool force)
        {
            bool running = IsGameRunning();
            if (running == gameRunning && !force) return;
            gameRunning = running;
            UpdateEnabled();
            if (working != null || busy) return;
            if (running) Say("Cry of Fear is running. Close it to install, update or uninstall.");
            else if (!force) { Say(""); RefreshInstallState(); }
        }

        void UpdateEnabled()
        {
            bool idle = working == null && !busy;
            foreach (MenuLine line in scriptLines) line.Enabled = idle && !gameRunning && ScriptsPresent();
            updateLine.Enabled = idle && (newerAsset == null || !gameRunning);
        }

        // ---- running the scripts (no console window) ----

        void RunScript(string files, string script, string args, string kind, string startNote)
        {
            if (working != null) return;
            if (IsGameRunning()) { CheckGame(true); EndUpdate(false); return; }
            string path = Path.Combine(files, script);
            if (!File.Exists(path)) { Say("The \"files\" folder is incomplete. Extract the whole ZIP again."); EndUpdate(false); return; }
            ProcessStartInfo psi = new ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "WindowsPowerShell\\v1.0\\powershell.exe"));
            // The same script and arguments as the .cmd files.
            psi.Arguments = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"" + path + "\"" + args;
            psi.UseShellExecute = false;
            psi.CreateNoWindow = true;
            psi.RedirectStandardOutput = true;
            psi.RedirectStandardError = true;
            psi.WorkingDirectory = files;
            psi.EnvironmentVariables["COF_LAUNCHER"] = "1";
            output = new List<string>();
            workingKind = kind;
            try
            {
                Process p = new Process();
                p.StartInfo = psi;
                p.OutputDataReceived += delegate(object s, DataReceivedEventArgs e) { if (e.Data != null) BeginInvoke(new Action<string, bool>(OnScriptLine), e.Data, false); };
                p.ErrorDataReceived += delegate(object s, DataReceivedEventArgs e) { if (e.Data != null) BeginInvoke(new Action<string, bool>(OnScriptLine), e.Data, true); };
                p.Start();
                working = p;
                p.BeginOutputReadLine();
                p.BeginErrorReadLine();
                // WaitForExit() without a timeout also waits for the last output line.
                Task.Run(delegate { p.WaitForExit(); int code = p.ExitCode; BeginInvoke(new Action<int>(ScriptDone), code); });
                ShowProgress(0, false);
                Say(startNote);
            }
            catch (Exception ex) { working = null; EndProgress(false); Say("Could not start: " + ex.Message); EndUpdate(false); }
            UpdateEnabled();
        }

        void OnScriptLine(string line, bool error)
        {
            output.Add((error ? "! " : "") + line);
            if (error) return;
            if (line.StartsWith("##cof-progress ", StringComparison.Ordinal))
            {
                Match m = Regex.Match(line, "^##cof-progress (\\d+) ?(.*)$");
                if (m.Success) { ShowProgress(int.Parse(m.Groups[1].Value), true); if (m.Groups[2].Value.Length > 0) Say(m.Groups[2].Value); }
                return;
            }
            string text = line.Trim();
            if (text.Length > 0) Say(text.Length > 150 ? text.Substring(0, 147) + "..." : text);
        }

        void ScriptDone(int code)
        {
            if (working != null) { working.Dispose(); working = null; }
            string log = WriteLog();
            bool ok = code == 0;
            EndProgress(ok);
            string kind = workingKind;
            UpdateEnabled();
            RefreshInstallState();
            if (!ok)
            {
                Say(FirstError() + (log != null ? " Details: " + log : ""));
                if (kind == "update") EndUpdate(false);
                return;
            }
            if (kind == "install") Say("Co-op " + version + " installed. Your saves are untouched.");
            else if (kind == "uninstall") Say("Co-op mod removed. Saves and playermodels kept.");
            else if (kind == "run") Say("Starting Cry of Fear... Host or join from the co-op menu.");
            else if (kind == "update") UpdateInstalled();
        }

        // The error PowerShell printed: "...\setup-coop.ps1 : message" plus
        // position lines; only the message is shown.
        string FirstError()
        {
            string message = "";
            foreach (string entry in output)
            {
                if (!entry.StartsWith("! ")) { if (message.Length > 0) break; continue; }
                string line = entry.Substring(2).Trim();
                if (line.Length == 0 || line.StartsWith("At ") || line.StartsWith("+ ") || line.StartsWith("CategoryInfo") || line.StartsWith("FullyQualifiedErrorId"))
                {
                    if (message.Length > 0) break;
                    continue;
                }
                int i = line.IndexOf(".ps1 : ", StringComparison.Ordinal);
                if (i >= 0) line = line.Substring(i + 7);
                message = message.Length > 0 ? message + " " + line : line;
            }
            if (message.Length == 0)
                for (int i = output.Count - 1; i >= 0 && message.Length == 0; i--)
                    if (!output[i].StartsWith("##cof-progress")) message = output[i].TrimStart('!', ' ');
            if (message.Length == 0) message = "Something went wrong.";
            return message.Length > 220 ? message.Substring(0, 217) + "..." : message;
        }

        string WriteLog()
        {
            string[] places = { Path.Combine(baseDir, "launcher.log"), Path.Combine(Path.GetTempPath(), "cof-coop-launcher.log") };
            foreach (string place in places)
            {
                try { File.WriteAllLines(place, output); return place == places[0] ? "launcher.log" : place; } catch { }
            }
            return null;
        }

        void Uninstall()
        {
            DialogResult answer = MessageBox.Show(this, "Remove the co-op mod from Cry of Fear?\n\nYour saves and your playermodels folder stay. Removed files are moved next to the game folder, not deleted.", Title, MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2);
            if (answer == DialogResult.Yes) RunScript(filesDir, "setup-coop.ps1", " -Uninstall", "uninstall", "Uninstalling...");
        }

        void OpenReadMe()
        {
            string readMe = Path.Combine(baseDir, "READ ME.txt");
            if (!File.Exists(readMe)) { Say("READ ME.txt is not next to this program."); return; }
            try { Process.Start(Path.Combine(Environment.SystemDirectory, "notepad.exe"), "\"" + readMe + "\""); } catch (Exception ex) { Say(ex.Message); }
        }

        // ---- updates (update-coop.ps1, then unpack and install) ----

        static int[] VersionParts(string text)
        {
            Match m = Regex.Match(text ?? "", "(?i)\\bv?(\\d+)\\.(\\d+)(?:\\.(\\d+))?");
            if (!m.Success) return null;
            return new int[] { int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value), m.Groups[3].Success ? int.Parse(m.Groups[3].Value) : 0 };
        }

        static int CompareVersions(int[] a, int[] b)
        {
            for (int i = 0; i < 3; i++) if (a[i] != b[i]) return Math.Sign(a[i] - b[i]);
            return 0;
        }

        static HttpClient MakeClient(int seconds)
        {
            ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;
            HttpClient client = new HttpClient();
            client.Timeout = TimeSpan.FromSeconds(seconds);
            client.DefaultRequestHeaders.UserAgent.ParseAdd("cof-coop-updater");
            return client;
        }

        void UpdateClicked()
        {
            if (busy || working != null) return;
            if (newerAsset != null) InstallUpdate(); else CheckForUpdates();
        }

        async void CheckForUpdates()
        {
            busy = true; UpdateEnabled();
            ShowProgress(0, false);
            Say("Checking GitHub for a newer version...");
            bool ok = false;
            try
            {
                int[] mine = VersionParts(version);
                string json;
                using (HttpClient client = MakeClient(15))
                {
                    client.DefaultRequestHeaders.Accept.ParseAdd("application/vnd.github+json");
                    json = await client.GetStringAsync(ReleaseApi);
                }
                Dictionary<string, object> release = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(json);
                string latest = Str(Get(release, "tag_name"));
                int[] theirs = VersionParts(latest);
                if (mine == null || theirs == null) { Say("Could not check for updates: unexpected version names. Nothing was changed."); return; }
                ok = true;
                if (CompareVersions(theirs, mine) <= 0) { Say("You have the newest version (" + version + ")."); return; }
                releasePage = Str(Get(release, "html_url"));
                newerAsset = null; newerTag = latest;
                IEnumerable assets = Get(release, "assets") as IEnumerable;
                if (assets != null)
                    foreach (object a in assets)
                    {
                        Dictionary<string, object> asset = a as Dictionary<string, object>;
                        if (asset == null) continue;
                        if (Str(Get(asset, "name")).EndsWith(".zip", StringComparison.OrdinalIgnoreCase) && Str(Get(asset, "browser_download_url")).StartsWith(DownloadPrefix, StringComparison.Ordinal)) { newerAsset = asset; break; }
                    }
                if (newerAsset == null) { Say("A newer version is out: " + latest + ". Opening its release page..."); OpenPage(releasePage); return; }
                updateLine.Text = "UPDATE TO " + latest;
                updateLine.Hint = "Downloads " + latest + ", unpacks it next to this folder and installs it. Saves stay.";
                Say("A newer version is out: " + latest + " (you have " + version + "). Click UPDATE TO " + latest + ".");
            }
            catch (Exception ex)
            {
                Exception inner = ex.InnerException ?? ex;
                Say("Could not check for updates (" + inner.Message.TrimEnd('.') + "). Check your internet and try later. Nothing was changed.");
            }
            finally { busy = false; EndProgress(ok); UpdateEnabled(); }
        }

        async void InstallUpdate()
        {
            if (IsGameRunning()) { CheckGame(true); return; }
            busy = true; UpdateEnabled();
            string zip = null;
            try
            {
                zip = await Download();
                ShowProgress(0, false);
                Say("Unpacking " + newerTag + "...");
                string target = NewFolder(Directory.GetParent(baseDir).FullName);
                if (target == null) target = NewFolder(DownloadsFolder());
                if (target == null) throw new IOException("there is no folder this program may write to");
                string zipPath = zip;
                await Task.Run(delegate { Unpack(zipPath, target); });
                string root = FindPackageRoot(target);
                if (root == null) throw new IOException("the downloaded ZIP is not a co-op package");
                int[] got = VersionParts(Str(Get(ReadRelease(Path.Combine(root, "files")), "coopFixes", "id")));
                int[] want = VersionParts(newerTag);
                if (got == null || want == null || CompareVersions(got, want) != 0) throw new IOException("the downloaded ZIP holds a different version than " + newerTag);
                CopyPlayerModels(Path.Combine(baseDir, "playermodels"), Path.Combine(root, "playermodels"));
                updateRoot = root;
                busy = false;
                RunScript(Path.Combine(root, "files"), "setup-coop.ps1", "", "update", "Installing " + newerTag + "...");
            }
            catch (Exception ex)
            {
                Exception inner = ex.InnerException ?? ex;
                busy = false;
                EndProgress(false);
                Say("Update failed (" + inner.Message.TrimEnd('.') + ")." + (zip != null ? " The download is kept in " + Path.GetDirectoryName(zip) + "." : " Get it from the release page instead."));
                if (zip == null) OpenPage(releasePage);
                UpdateEnabled();
            }
        }

        // Only the zip of this project's own release, to the Downloads folder.
        async Task<string> Download()
        {
            string url = Str(Get(newerAsset, "browser_download_url"));
            long size = Convert.ToInt64(Get(newerAsset, "size") ?? 0L);
            string digest = Str(Get(newerAsset, "digest"));
            string downloads = DownloadsFolder();
            string name = Regex.Replace(Path.GetFileName(Str(Get(newerAsset, "name"))), "[^A-Za-z0-9 ._()-]", "_");
            string target = Path.Combine(downloads, name);
            for (int n = 1; File.Exists(target); n++) target = Path.Combine(downloads, Path.GetFileNameWithoutExtension(name) + " (" + n + ")" + Path.GetExtension(name));
            string partial = target + ".part";
            ShowProgress(0, true);
            try
            {
                using (HttpClient client = MakeClient(300))
                using (HttpResponseMessage response = await client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead))
                {
                    response.EnsureSuccessStatusCode();
                    using (Stream input = await response.Content.ReadAsStreamAsync())
                    using (FileStream file = new FileStream(partial, FileMode.CreateNew))
                    using (SHA256 hash = SHA256.Create())
                    {
                        byte[] buffer = new byte[81920];
                        long done = 0; int read, lastPercent = -1;
                        while ((read = await input.ReadAsync(buffer, 0, buffer.Length)) > 0)
                        {
                            await file.WriteAsync(buffer, 0, read);
                            hash.TransformBlock(buffer, 0, read, null, 0);
                            done += read;
                            int percent = size > 0 ? (int)(done * 100 / size) : 0;
                            if (percent != lastPercent) { lastPercent = percent; ShowProgress(percent, size > 0); Say("Downloading " + newerTag + " (" + percent + "%)"); }
                        }
                        hash.TransformFinalBlock(new byte[0], 0, 0);
                        if (size > 0 && done != size) throw new IOException("the download is incomplete");
                        // GitHub lists "sha256:<hex>" for newer release files.
                        if (digest.StartsWith("sha256:", StringComparison.OrdinalIgnoreCase) && !string.Equals(digest.Substring(7), Hex(hash.Hash), StringComparison.OrdinalIgnoreCase)) throw new IOException("the download is damaged");
                    }
                }
                File.Move(partial, target);
                return target;
            }
            catch
            {
                try { if (File.Exists(partial)) File.Delete(partial); } catch { }
                throw;
            }
        }

        static string DownloadsFolder()
        {
            string downloads = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads");
            return Directory.Exists(downloads) ? downloads : Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
        }

        // "Cry of Fear Co-op <tag>" in parent, created empty; null when parent is not writable.
        string NewFolder(string parent)
        {
            try
            {
                string name = "Cry of Fear Co-op " + Regex.Replace(newerTag, "[^A-Za-z0-9._-]", "_");
                string path = Path.Combine(parent, name);
                for (int n = 2; Directory.Exists(path) || File.Exists(path); n++) path = Path.Combine(parent, name + " (" + n + ")");
                Directory.CreateDirectory(path);
                string probe = Path.Combine(path, ".write-test");
                File.WriteAllText(probe, "x"); File.Delete(probe);
                return path;
            }
            catch { return null; }
        }

        static void Unpack(string zip, string target)
        {
            string root = Path.GetFullPath(target).TrimEnd('\\') + "\\";
            using (ZipArchive archive = ZipFile.OpenRead(zip))
            {
                // Nothing may land outside the new folder.
                foreach (ZipArchiveEntry entry in archive.Entries)
                {
                    string dest = Path.GetFullPath(Path.Combine(root, entry.FullName.Replace('/', '\\')));
                    if (!dest.StartsWith(root, StringComparison.OrdinalIgnoreCase)) throw new IOException("the ZIP holds an unsafe path");
                }
                foreach (ZipArchiveEntry entry in archive.Entries)
                {
                    string dest = Path.GetFullPath(Path.Combine(root, entry.FullName.Replace('/', '\\')));
                    if (entry.Name.Length == 0) { Directory.CreateDirectory(dest); continue; }
                    Directory.CreateDirectory(Path.GetDirectoryName(dest));
                    entry.ExtractToFile(dest, false);
                }
            }
        }

        static string FindPackageRoot(string folder)
        {
            List<string> candidates = new List<string>();
            candidates.Add(folder);
            candidates.AddRange(Directory.GetDirectories(folder));
            foreach (string c in candidates)
                if (File.Exists(Path.Combine(c, "files\\setup-coop.ps1")) && File.Exists(Path.Combine(c, "files\\release.json"))) return c;
            return null;
        }

        // The player's own models come along; nothing in the old folder is removed.
        static void CopyPlayerModels(string from, string to)
        {
            if (!Directory.Exists(from)) return;
            foreach (string file in Directory.GetFiles(from, "*", SearchOption.AllDirectories))
            {
                string dest = Path.Combine(to, file.Substring(from.Length).TrimStart('\\'));
                if (File.Exists(dest)) continue;
                Directory.CreateDirectory(Path.GetDirectoryName(dest));
                File.Copy(file, dest);
            }
        }

        void UpdateInstalled()
        {
            string exe = Path.Combine(updateRoot, ExeName);
            if (File.Exists(exe))
            {
                Say("Updated to " + newerTag + ". Starting the new launcher...");
                try
                {
                    ProcessStartInfo psi = new ProcessStartInfo(exe);
                    psi.WorkingDirectory = updateRoot;
                    Process.Start(psi);
                    Timer close = new Timer();
                    close.Interval = 1200;
                    close.Tick += delegate { close.Stop(); Close(); };
                    close.Start();
                    return;
                }
                catch { }
            }
            Say("Updated to " + newerTag + ". Use the new folder from now on: " + updateRoot);
            try { Process.Start(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Windows), "explorer.exe"), "\"" + updateRoot + "\""); } catch { }
        }

        void EndUpdate(bool ok)
        {
            if (ok) return;
            busy = false;
            UpdateEnabled();
        }

        static void OpenPage(string url)
        {
            if (string.IsNullOrEmpty(url) || !url.StartsWith("https://github.com/", StringComparison.Ordinal)) url = ReleasesPage;
            try { Process.Start(url); } catch { }
        }
    }
}
