using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_JsonJpathSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_JsonJpathSearch), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_JsonJpathSearch));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(JPath), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(FilePath), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_JsonJpathSearch() : base(SymbolDefinition)
        { }

        public PSW_JsonJpathSearch(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "jpt")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string JPath
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string FilePath
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }
    }
}
