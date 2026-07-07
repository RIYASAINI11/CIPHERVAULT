const express = require("express");
const multer = require("multer");
const cors = require("cors");
const fs = require("fs");
const path = require("path");
const os = require("os");
const { execFile } = require("child_process");

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// ---------------- Folder Paths ----------------

const uploadDir = path.join(__dirname, "uploads");
const outputDir = path.join(__dirname, "output");
const frontendDir = path.join(__dirname, "..", "FRONTEND");

if (!fs.existsSync(uploadDir))
    fs.mkdirSync(uploadDir);

if (!fs.existsSync(outputDir))
    fs.mkdirSync(outputDir);

// ---------------- Multer ----------------

const storage = multer.diskStorage({

    destination: function(req,file,cb){

        cb(null,uploadDir);

    },

    filename:function(req,file,cb){

        cb(null,Date.now()+"_"+file.originalname);

    }

});

const upload=multer({storage});

// ---------------- cipher.exe ----------------

const cipherPath = path.join(
    __dirname,
    "..",
    "C_PROGRAM",
    "cipher.exe"
);

function deleteFileIfExists(filePath) {
    fs.unlink(filePath, (err) => {
        if (err && err.code !== "ENOENT") {
            console.log("Cleanup Error:", err.message);
        }
    });
}

// ---------------- Frontend ----------------

app.use(express.static(frontendDir));

app.get("/",(req,res)=>{

    res.sendFile(path.join(frontendDir, "Index.html"));

});

function getLocalIpAddresses() {
    const interfaces = os.networkInterfaces();
    const addresses = [];

    Object.values(interfaces).forEach((items) => {
        items.forEach((item) => {
            if (item.family === "IPv4" && !item.internal) {
                addresses.push(item.address);
            }
        });
    });

    return addresses;
}

app.post("/process", upload.single("file"), (req, res) => {

    try {

        if (!req.file) {
            return res.status(400).send("No file selected.");
        }

        const mode = req.body.mode;
        const password = req.body.password;

        if (!mode || !password) {
            return res.status(400).send("Mode or Password Missing.");
        }

        const inputFile = req.file.path;

        const extension = path.extname(req.file.originalname);

        let outputFile;

        if(mode==="encrypt"){

            outputFile=path.join(
                outputDir,
                Date.now()+".cv"
            );

        }
        else{

            outputFile=path.join(
                outputDir,
                Date.now()+extension
            );

        }
        console.log("Running cipher.exe...");
console.log("Mode:", mode);
console.log("Input:", inputFile);
console.log("Output:", outputFile);
console.log("Password received");

        execFile(

            cipherPath,

            [

                mode,

                inputFile,

                outputFile,

                password

            ],

            (error,stdout,stderr)=>{
                console.log("Callback Reached");
                if(error){

                    console.log(stderr);
                    deleteFileIfExists(inputFile);
                    deleteFileIfExists(outputFile);

                    return res.status(500).send(stdout);

                }

               console.log("Output File Exists:", fs.existsSync(outputFile));
console.log("Sending:", outputFile);

res.download(outputFile, (err) => {
    deleteFileIfExists(inputFile);
    deleteFileIfExists(outputFile);

    if (err) {
        console.log("Download Error:", err);
    } else {
        console.log("Download Sent Successfully");
    }
});

            }

        );

    }

    catch(err){

        console.log(err);

        res.status(500).send("Internal Server Error");

    }

});
app.listen(PORT, "0.0.0.0", ()=>{

    console.log("====================================");

    console.log(" CipherVault Server Started");

    console.log("====================================");

    console.log("Running On:");

    console.log("http://localhost:3000");

    getLocalIpAddresses().forEach((ip) => {
        console.log(`http://${ip}:${PORT}`);
    });

});
