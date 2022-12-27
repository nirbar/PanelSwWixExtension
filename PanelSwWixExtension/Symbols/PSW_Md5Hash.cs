using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_Md5Hash : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_Md5Hash), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_Md5Hash));
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
                    new ColumnDefinition(nameof(Plain), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_Md5Hash() : base(SymbolDefinition)
        { }

        public PSW_Md5Hash(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "md5")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Plain
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }
    }
}
