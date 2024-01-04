<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Fuzzys - CS2服务器排名</title>
    <link href="favicon.ico" rel="shortcut icon" type="image/x-icon">
    <meta name='robots' content='index, follow, max-image-preview:large, max-snippet:-1, max-video-preview:-1' />
    <meta name="description" content="List of player ranks" />
    <script src="https://cdn.jsdelivr.net/npm/sweetalert2@11.10.0/dist/sweetalert2.all.min.js"></script>
    <link href="https://cdn.jsdelivr.net/npm/sweetalert2@11.10.0/dist/sweetalert2.min.css" rel="stylesheet">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.7.1/jquery.min.js" integrity="sha512-v2CJ7UaYy4JwqLDIrZUI/4hqeoQieOmAZNXBeQyjo21dadnwR+8ZaIJVT8EE2iyI61OV8e6M8PP2/4hpQINQ/g==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
<?php
// Include header.php
include 'header.php';

// Include connection.php for connection details
include 'src/k4con.php';

// Connect to the database corresponding to the selected server
$selectedServer = isset($_GET['server']) ? $_GET['server'] : null;
$conn = connectToDatabase($selectedServer);

// Check if the connection was successfully established
if ($conn) {
    // Retrieve the list of servers after the connection is established
    $serverList = getServerList($conn);

    // Set the default server if it is not already set
    if (!isset($_GET['server'])) {
        header("Location: ?server=" . $conn->defaultServer);
        exit();
    }

    // Redirect to the default server if the 'server' parameter is not valid
    if (!in_array($_GET['server'], $serverList)) {
        header("Location: ?server=" . $conn->defaultServer);
        exit();
    }
} else {
    // Handle the error or return an appropriate value
    die("Database connection failed.");
}
?>

	
<script>
$(document).ready(function(){
    jQuery(".cell1").click(function() {
		var btn_call_action = jQuery(this);

        var data_steamid = jQuery(this).attr("data-steamid");
		var data_names = jQuery(this).attr("data-names");
		var data_kills = jQuery(this).attr("data-kills");
		var data_deaths = jQuery(this).attr("data-deaths");
		var data_assists = jQuery(this).attr("data-assists");
		var data_hitstaken = jQuery(this).attr("data-hitstaken");
		var data_hitsgiven = jQuery(this).attr("data-hitsgiven");
		var data_headshots = jQuery(this).attr("data-headshots");
		var data_grenades = jQuery(this).attr("data-grenades");
		var data_mvp = jQuery(this).attr("data-mvp");
		var data_roundwin = jQuery(this).attr("data-roundwin");
		var data_roundlose = jQuery(this).attr("data-roundlose");
		var data_kda = jQuery(this).attr("data-kda");	
	
  Swal.fire({
	html: '<h2>详细信息</h2><b>Steam 页面:</b> <a href="https://steamcommunity.com/profiles/' + data_steamid + '" target="_blank" rel="noopener">' + data_names + '</a><br><b>击杀:</b> ' + data_kills + '<br><b>死亡:</b> ' + data_deaths + '<br><b>助攻:</b> ' + data_assists + '<br><b>被击中:</b> ' + data_hitstaken + '<br><b>击中次数:</b> ' + data_hitsgiven + '<br><b>爆头:</b> ' + data_headshots + '<br><b>炸死:</b> ' + data_grenades + '<br><b>MVP:</b> ' + data_mvp + '<br><b>获胜回合:</b> ' + data_roundwin + '<br><b>落败回合:</b> ' + data_roundlose + '<br><b>KDA:</b> ' + data_kda,
	confirmButtonText: '关闭'
})
    });
});
</script>

<body>

<div class="server-buttons">
    <?php
    $serverList = getServerList($conn);
    foreach ($serverList as $server) {
        echo '<button class="server-button" onclick="setPrefixAndChangeServer(\'' . $server . '\', \'\')">' . strtoupper($server) . '</button>';
    }
    ?>
</div>

<script>
    document.addEventListener('DOMContentLoaded', function () {
        // Set the active class based on the current server parameter
        var searchParams = new URLSearchParams(window.location.search);
        var currentServer = searchParams.get('server');
        setActiveButton(currentServer);
    });

function changeServer(selectedServer) {
    // Set the 'server' parameter to the new server and remove other parameters
    window.location.href = '?' + new URLSearchParams({ 'server': selectedServer }).toString();
}

function setPrefixAndChangeServer(selectedServer, prefix) {
    // Save the prefix in a cookie
    setCookie("prefix", prefix, 30);

    // Redirect to the selected server
    changeServer(selectedServer);
}

function setActiveButton(serverName) {
    var buttons = document.querySelectorAll('.server-button');
    buttons.forEach(function (button) {
        button.classList.remove('active');
        if (button.innerText.toLowerCase() === serverName.toLowerCase()) {
            button.classList.add('active');
        }
    });
}

    function setCookie(cname, cvalue, exdays) {
        var d = new Date();
        d.setTime(d.getTime() + (exdays * 24 * 60 * 60 * 1000));
        var expires = "expires=" + d.toUTCString();
        document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
    }	
	
</script>


    <h1>玩家积分段位排行榜!</h1>

<!-- Search form for names -->
<div class="searchdiv">
    <form method="GET" action="">
        <input type="text" id="search" name="search" placeholder="输入玩家名字...">
        <input type="hidden" name="server" value="<?php echo $selectedServer; ?>">
        <button type="submit">搜索</button>
    </form>
    <?php
    // Check if a search has been performed
    if (isset($_GET['search'])) {
        echo '<a href="' . $_SERVER['PHP_SELF'] . '?server=' . $selectedServer . '" class="back-button">返回全部排名</a>';
    }
    ?>
</div><br>

    <?php
    // Set the number of records per page
    $recordsPerPage = 15;

    // Set the number of visible pages in pagination
    $visiblePages = 3;

    // Current page (default is 1 if not set)
    $current_page = isset($_GET['page']) ? $_GET['page'] : 1;

    // Calculate the offset to retrieve the correct records for the current page
    $offset = ($current_page - 1) * $recordsPerPage;

// The prefix is set based on the selected server.
$prefix = $conn->servers[$selectedServer]['prefix'];

	$search = isset($_GET['search']) ? $_GET['search'] : '';
    // Build the query with the prefix.
    $query = "SELECT {$prefix}k4ranks.name, {$prefix}k4ranks.rank, {$prefix}k4ranks.steam_id, {$prefix}k4ranks.points, {$prefix}k4stats.steam_id, {$prefix}k4stats.name, {$prefix}k4stats.kills, {$prefix}k4stats.deaths, {$prefix}k4stats.assists, {$prefix}k4stats.hits_taken, {$prefix}k4stats.hits_given, {$prefix}k4stats.headshots, {$prefix}k4stats.grenades, {$prefix}k4stats.mvp, {$prefix}k4stats.round_win, {$prefix}k4stats.round_lose, {$prefix}k4stats.kda 
        FROM {$prefix}k4ranks
        JOIN {$prefix}k4stats ON {$prefix}k4ranks.steam_id = {$prefix}k4stats.steam_id
        WHERE {$prefix}k4ranks.name LIKE :search OR {$prefix}k4stats.name LIKE :search
        ORDER BY {$prefix}k4ranks.points DESC, {$prefix}k4stats.kills DESC
        LIMIT :offset, :limit";

	$stmt = $conn->prepare($query);
	$stmt->bindParam(':limit', $recordsPerPage, PDO::PARAM_INT);
	$stmt->bindParam(':offset', $offset, PDO::PARAM_INT);
	$stmt->bindValue(':search', "%$search%", PDO::PARAM_STR);
	$stmt->execute();
	$result = $stmt->fetchAll(PDO::FETCH_ASSOC);

    if ($result) {
        // Display the beginning part of the wrapper
        echo '<div class="wrapper">';

        // Display the beginning part of the table
        echo '<div class="table">
                <div class="row header">
                    <div class="cell">#</div>
                    <div class="cell">名字</div>
                    <div class="cell">段位</div>
                    <div class="cell">分数</div>
                </div>';

        // Calculate the total number of records (not just those on the current page)
        $countQuery = "SELECT COUNT(*) as total FROM {$prefix}k4ranks WHERE name LIKE :search";
        $countStmt = $conn->prepare($countQuery);
        $countStmt->bindValue(':search', "%$search%", PDO::PARAM_STR);
        $countStmt->execute();
        $totalCount = $countStmt->fetchColumn();

        // Calculate the total number of pages
        $totalPages = ceil($totalCount / $recordsPerPage);

        // Calculate the range of visible pages based on the current page
        $startPage = max(1, $current_page - floor($visiblePages / 2));
        $endPage = min($totalPages, $startPage + $visiblePages - 1);

        // Calculate the order number of the first row on the page
        $startRowNumber = ($current_page - 1) * $recordsPerPage + 1;

        // Iterate through the results and display the information within the HTML template
        foreach ($result as $row) {
            // Assign a corresponding CSS class based on the rank name
            $rankClass = getRankClass($row["rank"]);

            echo '<div class="row">
                    <div class="cell" data-title="#">' . $startRowNumber . '</div>
                    <div class="cell cell1" data-title="名字" data-steamid="' . $row["steam_id"] . '" data-names="' . $row["name"] . '" data-kills="' . $row["kills"] . '" data-deaths="' . $row["deaths"] . '" data-assists="' . $row["assists"] . '" data-hitstaken="' . $row["hits_taken"] . '" data-hitsgiven="' . $row["hits_given"] . '" data-headshots="' . $row["headshots"] . '" data-grenades="' . $row["grenades"] . '" data-mvp="' . $row["mvp"] . '" data-roundwin="' . $row["round_win"] . '" data-roundlose="' . $row["round_lose"] . '" data-kda="' . $row["kda"] . '" >' . $row["name"] . '</div>
                    <div class="cell" data-title="段位" style="color: ' . $rankClass . '; font-weight: bold;">' . $row["rank"] . '</div>
                    <div class="cell" data-title="分数">' . $row["points"] . '</div>
                  </div>';

            $startRowNumber++;
        }

        // Display the ending part of the table
        echo '</div>';

// Display pagination buttons with improved CSS
echo '<div class="pagination">';
// Button for the first page
if ($startPage > 1) {
    echo '<a href="?page=1&server=' . $selectedServer;
    if (!empty($search)) {
        echo '&search=' . $search;
    }
    echo '">1</a>';
    if ($startPage > 2) {
        echo '<span>...</span>';
    }
}

// Display the buttons for each page in the calculated range
for ($i = $startPage; $i <= $endPage; $i++) {
    $activeClass = ($current_page == $i) ? 'active' : '';
    echo '<a href="?page=' . $i . '&server=' . $selectedServer;
    if (!empty($search)) {
        echo '&search=' . $search;
    }
    echo '" class="' . $activeClass . '">' . $i . '</a>';
}

// Button for the last page
if ($endPage < $totalPages) {
    if ($endPage < $totalPages - 1) {
        echo '<span>...</span>';
    }
    echo '<a href="?page=' . $totalPages . '&server=' . $selectedServer;
    if (!empty($search)) {
        echo '&search=' . $search;
    }
    echo '">' . $totalPages . '</a>';
}
echo '</div>';


        // Close the wrapper
        echo '</div>';
    } else {
    echo '<div class="errordiv">什么都没找到!' . $stmt->errorInfo()[2] . '</div>';
}

    // Function that returns the corresponding text color based on the rank name
    function getRankClass($rankName)
    {
        switch ($rankName) {
			
            case '无段位':
            return 'grey';
				
            case '白银':
            return '#C0C0C0';
				
            case '黄金':
            return 'gold';
				
            case '钻石':
            return 'blue';
			
            case '大师':
            return 'purple';
				
            case 'Boss':
            return 'magenta';
				
            case '超人':
            return 'Red';
				
            default:
            return '';
        }
    }

    // Close the connection to the database
    $conn = null;
    ?>
</body>
</html>