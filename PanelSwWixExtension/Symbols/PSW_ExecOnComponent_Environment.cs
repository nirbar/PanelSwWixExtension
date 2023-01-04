using System.Collections.Generic;
using System.Xml.Linq;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ExecOnComponent_Environment : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ExecOnComponent_Environment), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_ExecOnComponent_Environment));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(ExecOnId_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "PSW_ExecOnComponent", keyColumn: 1),
                    new ColumnDefinition(nameof(Name), ColumnType.String, 0, true, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Value), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_ExecOnComponent_Environment() : base(SymbolDefinition)
        { }

        public PSW_ExecOnComponent_Environment(SourceLineNumber lineNumber, Identifier execOnId) : base(SymbolDefinition, lineNumber, "eoe")
        {
            ExecOnId_ = execOnId.Id;
        }

        public string ExecOnId_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Name
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Value
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }
    }
}
