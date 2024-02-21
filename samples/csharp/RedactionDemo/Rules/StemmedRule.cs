using System.Text.RegularExpressions;

class StemmedRule : RegExRule
{
    private readonly string _stemmed;

    public StemmedRule(string text)
        : base($"\\b\\w*{_stemmer.Stem(text).Value}\\w*\\b")
    {
        _stemmed = _stemmer.Stem(text).Value;
    }

    public override bool IsMatch(Match match)
    {
        return string.Compare(_stemmer.Stem(match.Value).Value
            , _stemmed
            , StringComparison.OrdinalIgnoreCase) == 0;
    }

    private static readonly Porter2Stemmer.EnglishPorter2Stemmer _stemmer = new();
}

