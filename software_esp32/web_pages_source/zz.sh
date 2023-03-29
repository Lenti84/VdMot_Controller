echo "cleaning..."
rm -rf ../web_pages/*
mkdir ../web_pages
echo "gzipping files..."
gzip -9 -f -c ./h/index.html > ../web_pages/index.html
gzip -9 -f -c ./h/stmupdate.html > ../web_pages/stmupdate.html
gzip -9 -f -c ./h/favicon.ico > ../web_pages/favicon.ico
//cp ./h/index.html ../web_pages/index.html
echo "making tfs files..."
./mktfs ../web_pages
echo "moving tfs_data.c to .."
mv tfs_data.c ../src
echo "Done."
