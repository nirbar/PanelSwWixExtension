//-------------------------------------------------------------------------------------------------
// <copyright file="RelayCommand.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
// Base class that implements ICommand interface via delegates.
// This code came from the following MSDN article: http://msdn.microsoft.com/en-us/magazine/dd419663.aspx.
// </summary>
//-------------------------------------------------------------------------------------------------

namespace PanelSW.WixBA
{
    using System;
    using System.Diagnostics;
    using System.Windows.Input;

    /// <summary>
    /// Base class that implements ICommand interface via delegates.
    /// </summary>
    public class RelayCommand : ICommand
    {
        protected Action<object> executeAction;
        protected Predicate<object> canExecutePredicate;

        public RelayCommand(Action<object> execute)
            : this(execute, null)
        {
        }

        public RelayCommand(Action<object> execute, Predicate<object> canExecute)
        {
            this.executeAction = execute;
            this.canExecutePredicate = canExecute;
        }

        public event EventHandler CanExecuteChanged
        {
            add { CommandManager.RequerySuggested += value; }
            remove { CommandManager.RequerySuggested -= value; }
        }

        [DebuggerStepThrough]
        public bool CanExecute(object parameter)
        {
            return this.canExecutePredicate == null ? true : this.canExecutePredicate(parameter);
        }

        public void Execute(object parameter)
        {
            this.executeAction(parameter);
        }
    }
}
