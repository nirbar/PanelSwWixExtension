using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ShellExecute : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ShellExecute), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_ShellExecute));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Target), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Args), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Verb), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(WorkingDir), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Show), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Wait), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Flags), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_ShellExecute() : base(SymbolDefinition)
        { }

        public PSW_ShellExecute(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "shl")
        { }

        public string Target
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Args
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Verb
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string WorkingDir
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public int Show
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public int Wait
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public int Flags
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }

        public string Condition
        {
            get => Fields[6].AsString();
            set => this.Set(6, value);
        }
    }
}
