using System;
using System.ComponentModel;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using static ToolkitV.Models.TextureOptimization;

namespace ToolKitV.Views
{
    public partial class TextureOptimization : UserControl
    {
        public string MainPath { get; set; } = "";
        public string BackupPath { get; set; } = "";
        public string OptimizeSizeValue { get; set; } = "8192";
        public bool OnlyOverSizedToogled { get; set; } = false;
        public bool DownSizeValue { get; set; } = true;
        public bool FormatOptimizeValue { get; set; } = false;
        public TextureOptimization()
        {
            InitializeComponent();

            AnalyzeProgressHandler = AnalyzeProgressValue;
            OptimizeProgressHandler = OptimizeProgressValue;
        }

        private void UpdateData(StatsData data)
        {
            if (data.filesCount > 0)
            {
                Stats.FilesCount.Text = data.filesCount.ToString();
                Stats.OversizedCount.Text = data.oversizedCount.ToString();
                Stats.VirtualSize.Text = Math.Round(data.virtualSize, 2).ToString() + " MB";
                Stats.PhysicalSize.Text = Math.Round(data.physicalSize, 2).ToString() + " MB";
            }
        }

        public delegate void AnalyzeProgress(int progress);
        public Delegate AnalyzeProgressHandler;
        private void AnalyzeProgressValue(int progress)
        {
            Dispatcher.Invoke(() =>
            {
                AnalyzeButton.Progress.Width = Math.Ceiling((double)210 / 100 * progress);
            });
        }

        public delegate void OptimizeProgress(int progress);
        public Delegate OptimizeProgressHandler;
        private void OptimizeProgressValue(ResultsData data, int progress)
        {
            Dispatcher.Invoke(() =>
            {
                OptimizeButton.Progress.Width = Math.Ceiling((double)210 / 100 * progress);

                if (data.filesOptimized > 0)
                {
                    Stats.OptimizedFiles.Text = data.filesOptimized.ToString();
                    Stats.OptimizedSize.Text = Math.Round(data.optimizedSize, 2).ToString() + " MB";
                }
            });
        }
        private bool CheckCanBeProceed()
        {
            if (MainPath == "" || !System.IO.Directory.Exists(MainPath))
            {
                return false;
            }

            return true;
        }

        private void OnMainPathChanged(object sender, PropertyChangedEventArgs e)
        {
            MainPath = MainFolder.Path;


            if (CheckCanBeProceed())
            {
                OptimizeButton.IsButtonEnabled = true;
                AnalyzeButton.IsButtonEnabled = true;
            }
            else
            {
                OptimizeButton.IsButtonEnabled = false;
                AnalyzeButton.IsButtonEnabled = false;
            }
        }

        private void OnBackupPathChanged(object sender, PropertyChangedEventArgs e)
        {
            BackupPath = BackupFolder.Path;
        }

        private void OnOnlyOverSizedTexturesChanged(object sender, PropertyChangedEventArgs e)
        {
            OnlyOverSizedToogled = OnlyOverSized.IsToogled;
            //OptimizeSize.IsInputEnabled = !OnlyOverSized.IsToogled;
        }

        private void OptimizeSize_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            OptimizeSizeValue = OptimizeSize.Value;
        }

        private void Downsize_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            DownSizeValue = Downsize.IsToogled;
        }

        private void FormatOptimize_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            FormatOptimizeValue = FormatOptimize.IsToogled;
        }

        private async void AnalyzeButton_Click(object sender, RoutedEventArgs e)
        {
            OptimizeButton.IsButtonEnabled = false;
            AnalyzeButton.IsButtonEnabled = false;
            AnalyzeButton.Title = "In progress...";

            StatsData data = await Task.Run(() => GetStatsData(MainPath, AnalyzeProgressHandler));

            UpdateData(data);

            OptimizeButton.IsButtonEnabled = true;
            AnalyzeButton.IsButtonEnabled = true;
            AnalyzeButton.Title = "Analyze";
            AnalyzeButton.Progress.Width = 0;
        }

        private async void OptimizeButton_Click(object sender, RoutedEventArgs e)
        {
            if (!DownSizeValue && !FormatOptimizeValue)
            {
                return;
            }

            OptimizeButton.IsButtonEnabled = false;
            AnalyzeButton.IsButtonEnabled = false;
            OptimizeButton.Title = "In progress...";

            StatsData data = await Task.Run(() => GetStatsData(MainPath, null));

            UpdateData(data);

            await Task.Run(() => Optimize(MainPath, BackupPath, OptimizeSizeValue, OnlyOverSizedToogled, DownSizeValue, FormatOptimizeValue, OptimizeProgressHandler));

            OptimizeButton.IsButtonEnabled = true;
            AnalyzeButton.IsButtonEnabled = true;
            OptimizeButton.Title = "Optimize";
            OptimizeButton.Progress.Width = 0;

            StatsData newData = await Task.Run(() => GetStatsData(MainPath, null));

            Stats.FilesSizeResult.Text = Math.Round(newData.physicalSize, 2).ToString() + " MB";
            Stats.OptimizedProcent.Text = Convert.ToString(Math.Round(100 - (newData.physicalSize * 100 / data.physicalSize), 2)) + "%";
        }
    }
}