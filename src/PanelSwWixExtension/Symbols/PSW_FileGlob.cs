using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_FileGlob : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_FileGlob), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_FileGlob));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(Directory_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Directory", keyColumn: 1),
                    new ColumnDefinition(nameof(SourceDir), ColumnType.String, 72, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(Feature_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.None, keyTable: "Feature", keyColumn: 1),
                    new ColumnDefinition(nameof(ComponentGroup_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(PayloadGroup_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(PayloadPrefix), ColumnType.String, 72, false, true, ColumnCategory.Text, modularizeType: ColumnModularizeType.None),
                };
            }
        }

        public PSW_FileGlob() : base(SymbolDefinition)
        { }

        public PSW_FileGlob(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "glb")
        { }

        public string Directory_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string SourceDir
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Feature_
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string ComponentGroup_
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string PayloadGroup_
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public string PayloadPrefix
        {
            get => Fields[5].AsString();
            set => this.Set(5, value);
        }
    }
}
