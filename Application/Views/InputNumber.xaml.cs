using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace ToolKitV.Views
{
    public partial class InputNumber : UserControl, INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public string Title { get; set; } = "";
        public string Description { get; set; } = "";

        public bool IsInputEnabledValue { get; set; } = true;
        public bool IsInputEnabled
        {
            get => IsInputEnabledValue;
            set
            {
                if (value != IsInputEnabledValue)
                {
                    IsInputEnabledValue = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public string NumberValue { get; set; } = "";
        public string Value
        {
            get => NumberValue;
            set
            {
                if (value != NumberValue)
                {
                    NumberValue = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public InputNumber()
        {
            InitializeComponent();

            DataContext = this;
        }

        private static readonly Regex _regex = new Regex("[^0-9]"); // regex that matches disallowed text
        private static bool IsTextAllowed(string text)
        {
            return !_regex.IsMatch(text);
        }


        // Use the PreviewTextInputHandler to respond to key presses 
        private void PreviewTextInputHandler(object sender, TextCompositionEventArgs e)
        {
            e.Handled = Value.Length > 4 || !IsTextAllowed(e.Text);
        }

        // Use the DataObject.Pasting Handler 
        private void TextBoxPasting(object sender, DataObjectPastingEventArgs e)
        {
            if (e.DataObject.GetDataPresent(typeof(string)))
            {
                string text = (string)e.DataObject.GetData(typeof(string));
                if (Value.Length >= 4 || !IsTextAllowed(text))
                {
                    e.CancelCommand();
                }

                Value = text;
            }
            else
            {
                e.CancelCommand();
            }
        }
    }
}
