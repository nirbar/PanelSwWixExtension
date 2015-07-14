using System;
using System.Windows;
using System.Windows.Controls;
using System.Security;
using PanelSW.WixBA.Utils;

namespace PanelSW.WixBA.View
{
    /// <summary>
    /// Interaction logic for DbAccountView.xaml
    /// </summary>
    public partial class DbAccountView : BaseView
    {
        public DbAccountView(RootViewModel viewModel)
            : base(viewModel)
        {
            DataContext = viewModel;
            InitializeComponent();
        }

        public override ViewModelBase CurrentViewModel
        {
            get
            {
                return base.RootViewModel.DbAccountViewModel;
            }
        }

        internal String DbAccountPassword
        {
            get
            {
                String psw = null;
                Dispatcher.Invoke((Action)delegate()
                {
                    psw = _passwordBox.Password;
                }
                );

                return psw;
            }
        }
    }
}