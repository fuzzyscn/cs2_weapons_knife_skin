<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Toplay - CS2 RANK STATS</title>
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
		var data_roundwin = jQuery(this).attr("data-roundwin");
		var data_roundlose = jQuery(this).attr("data-roundlose");
		var data_gamewin = jQuery(this).attr("data-gamewin");
		var data_gamelose = jQuery(this).attr("data-gamelose");
		var data_firstblood = jQuery(this).attr("data-firstblood");
		var data_shoots = jQuery(this).attr("data-shoots");
		var data_mvp = jQuery(this).attr("data-mvp");


	
  Swal.fire({
	html: '<h2>STATS</h2><b>Steam Profile:</b> <a href="https://steamcommunity.com/profiles/' + data_steamid + '" target="_blank" rel="noopener">' + data_names + '</a><br><b>Kills:</b> ' + data_kills + '<br><b>Deaths:</b> ' + data_deaths + '<br><b>FirstBlood:</b> ' + data_firstblood + '<br><b>Shoots:</b> ' + data_shoots + '<br><b>Assists:</b> ' + data_assists + '<br><b>Hits Taken:</b> ' + data_hitstaken + '<br><b>Hits Given:</b> ' + data_hitsgiven + '<br><b>Headshots:</b> ' + data_headshots + '<br><b>Grenades:</b> ' + data_grenades + '<br><b>MVP:</b> ' + data_mvp + '<br><b>Rounds Win:</b> ' + data_roundwin + '<br><b>Rounds Lose:</b> ' + data_roundlose +' <br><b>Game Win:</b> ' + data_gamewin + '<br><b>Game Lose:</b> ' + data_gamelose,
	confirmButtonText: 'Close'
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


    <h1>List of player ranks!</h1>

<!-- Search form for names -->
<div class="searchdiv">
    <form method="GET" action="">
        <input type="text" id="search" name="search" placeholder="Enter name...">
        <input type="hidden" name="server" value="<?php echo $selectedServer; ?>">
        <button type="submit">Search</button>
    </form>
    <?php
    // Check if a search has been performed
    if (isset($_GET['search'])) {
        echo '<a href="' . $_SERVER['PHP_SELF'] . '?server=' . $selectedServer . '" class="back-button">Back to All</a>';
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
    $query = "SELECT {$prefix}k4ranks.name, {$prefix}k4ranks.rank, {$prefix}k4ranks.steam_id, {$prefix}k4ranks.points, {$prefix}k4stats.steam_id, {$prefix}k4stats.name, {$prefix}k4stats.kills, {$prefix}k4stats.deaths, {$prefix}k4stats.assists, {$prefix}k4stats.shoots, {$prefix}k4stats.hits_taken, {$prefix}k4stats.hits_given, {$prefix}k4stats.mvp, {$prefix}k4stats.headshots, {$prefix}k4stats.grenades, {$prefix}k4stats.game_win, {$prefix}k4stats.game_lose, {$prefix}k4stats.round_win, {$prefix}k4stats.round_lose, {$prefix}k4stats.firstblood
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
                    <div class="cell">Name</div>
                    <div class="cell">Rank</div>
                    <div class="cell">Points</div>
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
					<div class="cell cell1" data-title="Name" data-steamid="' . $row["steam_id"] . '" data-names="' . $row["name"] . '" data-kills="' . $row["kills"] . '" data-firstblood="' . $row["firstblood"] . '" data-deaths="' . $row["deaths"] . '" data-assists="' . $row["assists"] . '" data-shoots="' . $row["shoots"] . '" data-hitstaken="' . $row["hits_taken"] . '" data-hitsgiven="' . $row["hits_given"] . '" data-headshots="' . $row["headshots"] . '" data-grenades="' . $row["grenades"] . '" data-roundwin="' . $row["round_win"] . '" data-roundlose="' . $row["round_lose"] . '" data-gamewin="' . $row["game_win"] . '" data-mvp="' . $row["mvp"] . '" data-gamelose="' . $row["game_lose"] . '">' . $row["name"] . '</div>
                    <div class="cell" data-title="Rank" style="color: ' . $rankClass . '; font-weight: bold;">' . $row["rank"] . '</div>
                    <div class="cell" data-title="Points">' . $row["points"] . '</div>
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
    echo '<div class="errordiv">Nothing was found!' . $stmt->errorInfo()[2] . '</div>';
}

    // Function that returns the corresponding text color based on the rank name
    function getRankClass($rankName)
    {
        switch ($rankName) {
			
            case 'Bronze':
            return 'grey';
				
            case 'Silver':
            return '#C0C0C0';
				
            case 'Gold':
            return 'gold';
				
            case 'Platinum':
            return 'blue';
			
            case 'Diamond':
            return 'purple';
				
            case 'Master':
            return 'magenta';
				
            case 'GrandMaster':
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