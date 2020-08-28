using Microsoft.Win32;
using System;
using System.Windows;

using GifCLI;

namespace GifConvertorInterface
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        
        public MainWindow()
        {
            InitializeComponent();
        }

        private void OpenFile(object sender, RoutedEventArgs e)
        {
            var input = Browse();

            if (input == null)
                return;

            var output = FolderBrowse();

            if (output == null)
                return;

            string tmp = input.Substring(0, input.LastIndexOf("."));
            tmp = tmp.Substring(tmp.LastIndexOf("\\") + 1);


            output += "\\" + tmp +".gif";


            using (var convertor = new ConvertorExternal(input, output))
            {
                convertor.ToConvert();
            }

        }

        private string Browse()
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();

            openFileDialog.Filter =
                "All files (*.*)|*.*|" +
                "MP4 files (*.zip)|*.MP4;*.mp4";

            openFileDialog.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);

            if (openFileDialog.ShowDialog() == true)
            {
                return openFileDialog.FileName;
            }

            return null;
        }


        string FolderBrowse()
        {
            using (var dialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                System.Windows.Forms.DialogResult result = dialog.ShowDialog();

                if (result == System.Windows.Forms.DialogResult.OK)
                    return dialog.SelectedPath;
            }

            return null;
        }


    }
}
