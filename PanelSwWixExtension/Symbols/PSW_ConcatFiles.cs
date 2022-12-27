using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ConcatFiles : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ConcatFiles), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_ConcatFiles));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(RootFile_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(MyFile_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                    new ColumnDefinition(nameof(Size), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                };
            }
        }

        public PSW_ConcatFiles() : base(SymbolDefinition)
        { }

        public PSW_ConcatFiles(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "spl")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string RootFile_
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string MyFile_
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public int Order
        {
            get => Fields[3].AsNumber();
            set => Fields[3].Set(value);
        }

        public int Size
        {
            get => Fields[4].AsNumber();
            set => Fields[4].Set(value);
        }
    }
}
