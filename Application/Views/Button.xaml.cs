using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;

namespace ToolKitV.Views
{
    public partial class Button : UserControl, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        public event RoutedEventHandler Click;

        private void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public string TitleValue { get; set; }
        public string Title
        {
            get => TitleValue;
            set
            {
                if (value != TitleValue)
                {
                    TitleValue = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public bool IsButtonEnabledValue { get; set; } = true;
        public bool IsButtonEnabled
        {
            get => IsButtonEnabledValue;
            set
            {
                if (value != IsButtonEnabledValue)
                {
                    IsButtonEnabledValue = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public Button()
        {
            InitializeComponent();

            DataContext = this;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (IsButtonEnabled)
            {
                Click?.Invoke(sender, e);
            }
        }
    }
}
