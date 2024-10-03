# Basic setup (Linux)

`sudo apt install libjsoncpp-dev libcurl4-openssl-dev libxml2-dev`
Set the environment variables `CSE450BOTTOKEN` and `CANVASTOKEN` as your api keys for discord and canvas

If you want to find your course ID for a canvas class:
`sudo apt install jq`
`curl -H "Authorization: Bearer YOUR_CANVAS_TOKEN" "https://canvas.instructure.com/api/v1/courses?page=1&per_page=100_" | jq .`

# How does canvas fetching work?

## Monitors Assignments

It will track assignments from the /assignments endpoint
Filter out the entries with "grading_type" != "not_graded"
Use those to maintain a list of ungraded assignments

## Tracks Grading Status

For each ungraded assignment it will check its grading status via the assignmentID/submissions/self endpoint
If the graded_at value changes from null to a valid timestamp the bot will notify the user

## Sends Notifications

If a new assignment appears in the gradebook aka a new entry in list of ungraded assignments
or if an assignment's graded_at field is updated
it will send a notification to the configured discord channel