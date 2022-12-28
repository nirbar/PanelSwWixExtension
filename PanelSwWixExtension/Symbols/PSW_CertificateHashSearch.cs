using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_CertificateHashSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_CertificateHashSearch), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_CertificateHashSearch));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(CertName), ColumnType.Localized, 72, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(FriendlyName), ColumnType.Localized, 72, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Issuer), ColumnType.Localized, 72, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(SerialNumber), ColumnType.Localized, 72, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_CertificateHashSearch() : base(SymbolDefinition)
        { }

        public PSW_CertificateHashSearch(SourceLineNumber lineNumber, string property) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, property))
        { }

        public string CertName
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string FriendlyName
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Issuer
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string SerialNumber
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }
    }
}
