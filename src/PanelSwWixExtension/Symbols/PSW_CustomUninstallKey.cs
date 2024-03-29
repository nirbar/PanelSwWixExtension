using System.Collections.Generic;
using System.Xml.Linq;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_CustomUninstallKey : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_CustomUninstallKey), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_CustomUninstallKey));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(ProductCode), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Name), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Data), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(DataType), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Attributes), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 1, maxValue: 2),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_CustomUninstallKey() : base(SymbolDefinition)
        { }

        public PSW_CustomUninstallKey(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "cuk")
        { }

        public string ProductCode
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Name
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Data
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string DataType
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public int Attributes
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
