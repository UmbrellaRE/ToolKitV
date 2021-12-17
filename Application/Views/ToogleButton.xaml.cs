using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;

namespace ToolKitV.Views
{
    public partial class ToogleButton : UserControl, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public string Title { get; set; } = "";
        public string Description { get; set; } = "";

        public bool IsToogledValue { get; set; }
        public bool IsToogled
        {
            get => IsToogledValue;
            set
            {
                if (value != IsToogledValue)
                {
                    IsToogledValue = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public ToogleButton()
        {
            InitializeComponent();

            DataContext = this;
        }

        private void ToggleButton_Click(object sender, RoutedEventArgs e)
        {
            IsToogled = !IsToogled;
        }
    }
}
