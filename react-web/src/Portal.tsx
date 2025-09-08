import { Suspense } from "react";
import './Portal.css'
const fetchConfig = () => {
    let data;
    const promise = fetch("./api/portal.clasp")
        .then((response) => response.json())
        .then((json) => (data = json));
    return {
        read() {
            if (!data) {
                throw promise;
            }
            return data;
        },
    };
};

const fetchTimezones = () => {
    let data;
    const promise = fetch("./api/timezones.json")
        .then((response) => response.json())
        .then((json) => (data = json));
    return {
        read() {
            if (!data) {
                throw promise;
            }
            return data;
        },
    };
};
const configData = fetchConfig();
const timezoneData = fetchTimezones();
const TimezoneEntry = ({ name, offset}) => {
    return (<option value={offset}>{name}</option>
    )
}
const Timezones = () => {
    const tz = timezoneData.read();
    const tzArray = [];
    for (let i: number = 0; i < tz.length; ++i) {
        tzArray.push(<TimezoneEntry name={tz[i].value} offset={tz[i].offset} />);
    }
    return (<select name="tz">{tzArray}</select>)

};
const Configuration = () => {
    const cfg = configData.read();
    return (<>
        <label>SSID:</label><input type="text" name="ssid" defaultValue={cfg.ssid} /><br />
        <label>Password:</label><input type="text" name="pass" defaultValue={cfg.pass} /><br />
        <label>Timezone:</label><Timezones/>
    </>)
}
export default function Portal() {
    return (<Suspense fallback={<p>waiting for data...</p>}>
            <form method="GET">
                <Configuration />
                <input type="submit" name="Save" />
            </form>
            </Suspense>);
}
