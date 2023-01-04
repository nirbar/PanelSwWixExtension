using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_Telemetry : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_Telemetry), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_Telemetry));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Url), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Page), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Method), ColumnType.Localized, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Data), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Flags), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_Telemetry() : base(SymbolDefinition)
        { }

        public PSW_Telemetry(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "tlm")
        { }

        public string Url
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Page
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Method
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string Data
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public int Flags
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public string Condition
        {
            get => Fields[5].AsString();
            set => this.Set(5, value);
        }
    }
}
