using System.Collections.Generic;
using System.Xml;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_JsonJPath : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_JsonJPath), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_JsonJPath));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column)
                    , new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1)
                    , new ColumnDefinition(nameof(FilePath), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(File_), ColumnType.String, 72, true, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1)
                    , new ColumnDefinition(nameof(JPath), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Value), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Formatting), ColumnType.Number, 1, false, false, ColumnCategory.Integer, 0, 127)
                    , new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 2)
                };
            }
        }

        public PSW_JsonJPath() : base(SymbolDefinition)
        { }

        public PSW_JsonJPath(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "jpt")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string FilePath
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string File_
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string JPath
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public string Value
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public int Formatting
        {
            get => Fields[5].AsNumber();
            set => Fields[5].Set(value);
        }

        public int ErrorHandling
        {
            get => Fields[6].AsNumber();
            set => Fields[6].Set(value);
        }
    }
}
