using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_WebsiteConfig : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_WebsiteConfig), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_WebsiteConfig));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(Website), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Stop), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 1),
                    new ColumnDefinition(nameof(Start), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 1),
                    new ColumnDefinition(nameof(AutoStart), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: -1, maxValue: 1),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                };
            }
        }

        public PSW_WebsiteConfig() : base(SymbolDefinition)
        { }

        public PSW_WebsiteConfig(SourceLineNumber lineNumber, string id) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, id))
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Website
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public int Stop
        {
            get => Fields[2].AsNumber();
            set => this.Set(2, value);
        }

        public int Start
        {
            get => Fields[3].AsNumber();
            set => this.Set(3, value);
        }

        public int AutoStart
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public int ErrorHandling
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }
    }
}
