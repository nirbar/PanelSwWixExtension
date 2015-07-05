//-------------------------------------------------------------------------------------------------
// <copyright file="ViewModelBase.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
//-------------------------------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;
using System.Windows.Input;
using System.Windows;
using PanelSW.WixBA.View;
using System.Xml.XPath;
using System.IO;

namespace PanelSW.WixBA
{
    /// <summary>
    /// Validate that the machine is part of a domain 
    /// </summary>
    public class ViewModelBase : PropertyNotifyBase
    {
        protected RootViewModel _root;

        public ViewModelBase(RootViewModel root)
        {
            this._root = root;
        }

		#region Button1: "Close" by default

        public virtual ICommand Button1Command
        {
            get
            {
				return this._root.CloseCommand;
            }
        }
		
		public virtual Visibility Button1Visibility
		{
			get
			{
				return Visibility.Visible;
			}
		}
		
		public virtual object Button1Content
		{
			get
			{
				return "Close";
			}
		}
		
		#endregion

		#region Button2: N/A by default

        protected ICommand _button2Command;
        public virtual ICommand Button2Command
        {
            get
            {
                if( _button2Command == null)
                {
                    _button2Command = new RelayCommand(
                        (a) => 
                            {
                            },
                        (a) => 
                            true
                        );
                }

                return _button2Command;
            }
        }

		public virtual Visibility Button2Visibility
		{
			get
			{
				return Visibility.Hidden;
			}
		}
		
		public virtual object Button2Content
		{
			get
			{
				return "";
			}
		}

		#endregion

		#region Button3: N/A by default

        protected ICommand _button3Command;
        public virtual ICommand Button3Command
        {
            get
            {
                if( _button3Command == null)
                {
                    _button3Command = new RelayCommand(
                        (a) => 
                            {
                            },
                        (a) => 
                            true
                        );
                }

                return _button3Command;
            }
        }

		public virtual Visibility Button3Visibility
		{
			get
			{
				return Visibility.Hidden;
			}
		}
		
		public virtual object Button3Content
		{
			get
			{
				return "";
			}
		}

		#endregion

        #region Button4: N/A by default

        protected ICommand _button4Command;
        public virtual ICommand Button4Command
        {
            get
            {
                if (_button4Command == null)
                {
                    _button4Command = new RelayCommand(
                        (a) =>
                        {
                        },
                        (a) =>
                            true
                        );
                }

                return _button4Command;
            }
        }

        public virtual Visibility Button4Visibility
        {
            get
            {
                return Visibility.Hidden;
            }
        }

        public virtual object Button4Content
        {
            get
            {
                return "";
            }
        }

        #endregion
    }
}