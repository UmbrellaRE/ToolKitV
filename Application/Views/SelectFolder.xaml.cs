using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;
using WinForms = System.Windows.Forms;

namespace ToolKitV.Views
{
    public partial class SelectFolder : UserControl, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public string Title { get; set; } = "";

        public string PathValue { get; set; } = "";
        public string Path
        {
            get => PathValue;
            set
            {
                if (value != PathValue)
                {
                    PathValue = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public SelectFolder()
        {
            InitializeComponent();

            DataContext = this;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            WinForms.FolderBrowserDialog FBD = new WinForms.FolderBrowserDialog();
            if (FBD.ShowDialog() == WinForms.DialogResult.OK)
            {
                Path = FBD.SelectedPath;
            }
        }
    }
}
