using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using PanelSW.WixBA;
using PanelSW.WixBA.Utils;

namespace PanelSW.WixBA.View
{
    /// <summary>
    /// Interaction logic for InstallDirView.xaml
    /// </summary>
    public partial class InstallDirView : BaseView
    {
        public InstallDirView(RootViewModel viewModel)
            :base(viewModel)
        {
            DataContext = viewModel;
            InitializeComponent();
        }

        public override ViewModelBase CurrentViewModel
        {
            get
            {
                return base.RootViewModel.InstallDirViewModel;
            }
        }
    }
}
