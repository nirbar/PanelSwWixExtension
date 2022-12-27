using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ExecOn_ConsoleOutput : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ExecOn_ConsoleOutput), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_ExecOn_ConsoleOutput));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(ExecOnId_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "PSW_ExecOnComponent", keyColumn: 1),
                    new ColumnDefinition(nameof(Expression), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Flags), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 1),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                    new ColumnDefinition(nameof(PromptText), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_ExecOn_ConsoleOutput() : base(SymbolDefinition)
        { }

        public PSW_ExecOn_ConsoleOutput(SourceLineNumber lineNumber, Identifier execOnId) : base(SymbolDefinition, lineNumber, "std")
        {
            ExecOnId_ = execOnId.Id;
        }

        public string ExecOnId_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Expression
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public int Flags
        {
            get => Fields[2].AsNumber();
            set => Fields[2].Set(value);
        }

        public int ErrorHandling
        {
            get => Fields[3].AsNumber();
            set => Fields[3].Set(value);
        }

        public string PromptText
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }
    }
}
