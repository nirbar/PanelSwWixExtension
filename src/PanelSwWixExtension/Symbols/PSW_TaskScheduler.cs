using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_TaskScheduler : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_TaskScheduler), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_TaskScheduler));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(TaskName), ColumnType.Localized, 72, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(TaskXml), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(User), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Password), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_TaskScheduler() : base(SymbolDefinition)
        { }

        public PSW_TaskScheduler(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "tsk")
        { }

        public string TaskName
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Component_
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string TaskXml
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string User
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string Password
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }
    }
}
