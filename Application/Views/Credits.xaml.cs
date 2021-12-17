using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;

namespace ToolKitV.Views
{
    public partial class Credits : UserControl
    {
        private readonly string DiscordUrl = "https://discord.gg/8mEAy9a";
        private readonly string SiteUrl = "https://umbrella.re";
        public string Version { get; set; } = "";
        public Credits()
        {
            InitializeComponent();

            Version = "v" + GetType().Assembly.GetName().Version.ToString();

            DataContext = this;
        }

        private void OpenLink(string url)
        {
            Process openProcess = new Process();
            openProcess.StartInfo.FileName = "cmd";
            openProcess.StartInfo.Arguments = $"/c start {url}";
            openProcess.StartInfo.UseShellExecute = false;
            openProcess.StartInfo.CreateNoWindow = true;

            openProcess.Start();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            OpenLink(DiscordUrl);
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            OpenLink(SiteUrl);
        }
    }
}
