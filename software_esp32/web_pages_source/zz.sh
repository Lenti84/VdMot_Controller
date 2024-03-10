echo "cleaning..."
rm -rf ../web_pages/*
mkdir ../web_pages
echo "gzipping files..."
gzip -9 -f -c ./h/index.html > ../web_pages/index.html
gzip -9 -f -c ./h/stmupdate.html > ../web_pages/stmupdate.html
gzip -9 -f -c ./h/favicon.ico > ../web_pages/favicon.ico
gzip -9 -f -c ./h/window_close.png > ../web_pages/window_close.png
gzip -9 -f -c ./h/window_open.png > ../web_pages/window_open.png
gzip -9 -f -c ./h/window_gray.png > ../web_pages/window_gray.png
gzip -9 -f -c ./h/blank.png > ../web_pages/blank.png
gzip -9 -f -c ./h/warning_yellow.png > ../web_pages/warning_yellow.png
gzip -9 -f -c ./h/warning_red.png > ../web_pages/warning_red.png
//cp ./h/index.html ../web_pages/index.html
echo "making tfs files..."
./mktfs ../web_pages
echo "moving tfs_data.c to .."
mv tfs_data.c ../src
echo "Done."
