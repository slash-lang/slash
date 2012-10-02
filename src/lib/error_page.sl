<% \(err) { %>
<!DOCTYPE html>
<html>
<head>
    <title><%= err.class.to_s %> at <%= Request.uri %></title>
    <style>
    body {
        font-family:Verdana, sans-serif;
        margin:0;
        font-size:13px;
    }
    .container {
        margin:0px auto;
    }
    header, section {
        display:block;
        padding:32px 64px;
        margin:0;
    }
    header {
        background-color:#f9d4cf;
        border-bottom:1px solid #ea6756;
    }
    h2 {
        font-weight:normal;
        margin:0;
        margin-bottom:16px;
    }
    h2 span {
        color:#666666;
    }
    header table {
        border-collapse:collapse;
    }
    header table th {
        text-align:right;
        font-weight:bold;
        padding-right:12px;
    }
    header table th, header table td {
        padding:4px 6px;
    }
    .backtrace {
        background-color:#f0f0f0;
        border-bottom:1px solid #999999;
    }
    code {
        background-color:#f0f0f0;
        padding:2px 4px;
        border:1px solid #d0d0d0;
    }
    strong {
        font-size:12px;
    }
    li {
        margin-bottom:12px;
    }
    </style>
</head>
<body>
    <header>
        <h2><%= err.class.to_s %> <span>at <%= Request.uri %></span></h2>
        <table>
            <tr>
                <th>Message</th>
                <td><%= err.message %></td>
            </tr>
            <% if err.backtrace.any { %>
                <tr>
                    <th>File</th>
                    <td><%= err.backtrace[0].file %></td>
                </tr>
                <tr>
                    <th>Line</th>
                    <td><%= err.backtrace[0].line %></td>
                </tr>
            <% } %>
        </table>
    </header>
    <section class="backtrace">
        <h2>Backtrace</h2>
        <ul>
            <% for frame in err.backtrace { %>
                <li>
                    <code>#<%= frame.method %></code> in <strong><%= frame.file %></strong>, line <strong><%= frame.line %></strong>
                </li>
            <% } %>
        </ul>
    </section>
    <section class="info">
        <p>
            You are seeing this page because Slash is set to show descriptive error pages whenever an exception is not caught.
        </p>
        <p>
            To disable descriptive error pages, set <code>Response.descriptive_error_pages</code> to <code>false</code>
        </p>
    </section>
</body>
</html>
<% } %>