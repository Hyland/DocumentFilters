using System.Text.Json.Serialization;

public interface IRule
{
    IEnumerable<(int From, int To)> Match(string body);
}

public class Rule
{
    public enum MatchType
    {
        Exact,
        RegEx,
        Contains,
        Stemmed,
    }

    public static IRule Make(string text, MatchType matchType) => matchType switch
    {
        MatchType.Exact => new EqualsRule(text),
        MatchType.Contains => new ContainsRule(text),
        MatchType.RegEx => new RegExRule(text),
        MatchType.Stemmed => new StemmedRule(text),
        _ => throw new ArgumentException($"Unknown Match Type: {matchType}", "matchType")
    };

    [JsonPropertyName("name")]
    public string? Name { get; set; }
    [JsonPropertyName("category")]
    public string? Category { get; set; }
    [JsonPropertyName("regex")]
    public string? RegEx { get; set; }
}
